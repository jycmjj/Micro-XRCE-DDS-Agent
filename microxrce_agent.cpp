// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <uxr/agent/AgentInstance.hpp>

#include <uxr/agent/xrcedds_demo.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "Student.h"
#include "fileInfo.h"
#include <atomic>
#include <memory>
#include "ucdr/microcdr.h"


#define PORT 2024

using namespace std;
struct AgentMesg
{
    int argcAgent;
    char **argvAgent;
};

mutex mtx;
condition_variable cv;
vector<Student> students;
bool saveFlag = false;
bool done = false;

bool compareStudents(const Student& s1, const Student& s2) {
    if (s1.number != s2.number) {
        return s1.number < s2.number;
    } else if (s1.grade > s2.grade) {
        return s2.number;
    } else {
        return s1.number;
    }
}

string getCurrentTime() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", ltm);
    return string(buf);
}

unsigned long long getCurrentTimestamp() {
    return static_cast<unsigned long long>(time(0));
}

fileInfo saveToFile(const vector<Student>& students) {
    string filename = "../file/" + getCurrentTime() + ".txt";
    ofstream outfile(filename);
    for (const auto& student : students) {
        outfile << student.name << " " << student.number << " " << student.grade << " ";
        for (int i = 0; i < 3; ++i) {
            outfile << student.hobby[i] << " ";
        }
        outfile << endl;
    }
    outfile.close();

    fileInfo fileInfo;
    strncpy(fileInfo.filename, filename.c_str(), sizeof(fileInfo.filename) - 1);
    fileInfo.filename[sizeof(fileInfo.filename) - 1] = '\0'; // Ensure null-termination
    fileInfo.timestamp = getCurrentTimestamp();

    return fileInfo;
}

void fileSaver() {
    while (true) {
        vector<Student> local_students;
        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [] { return saveFlag || done; });
            if (done && students.empty()) {
                break;
            }
            if (saveFlag) {
                local_students.swap(students);
                saveFlag = false;
            }
        }
        if (!local_students.empty()) {
            sort(local_students.begin(), local_students.end(), compareStudents);
            saveToFile(local_students);
            local_students.clear();
        }
    }
}

Student deserializeStudent(const char* buffer) { // 反序列化
    Student student3;
    char name[50], hobby1[30], hobby2[30], hobby3[30];
    int fields = sscanf(buffer, "%29[^,],%ld,%ld,%29[^,],%29[^,],%29[^,]", name, &student3.number, &student3.grade, hobby1, hobby2, hobby3);
    student3.name = string(name);
    student3.hobby[0] = hobby1;
    student3.hobby[1] = hobby2;
    student3.hobby[2] = hobby3;
    return student3;
}
void receiveData() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[256] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket 失败");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt 失败");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind 错误");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen 错误");
        exit(EXIT_FAILURE);
    }

    vector<thread> client_threads; // 存放客户端线程的向量

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept 错误");
            exit(EXIT_FAILURE);
        }
        // 创建一个新线程来处理每个客户端
        client_threads.emplace_back([new_socket]() {
            char buffer[256] = {0};
            int valread = read(new_socket, buffer, sizeof(buffer));
            if (valread < 0) {
                perror("读取错误");
            } else if (valread > 0) {
                Student student = deserializeStudent(buffer);
                cout << "收到学生数据: " << student.name << " " << student.number << " " << student.grade << " ";
                for (int i = 0; i < 3; ++i) {
                    cout << student.hobby[i] << " ";
                }
                cout << endl;
                {
                    lock_guard<mutex> lock(mtx);
                    students.push_back(student);
                    if (students.size() >= 1000) {
                        saveFlag = true;
                    }
                }
                cv.notify_one();

            } else {
                cerr << "接收到不完整的学生数据" << endl;
            }
            close(new_socket);
        });
    }

    // 加入所有客户端线程（尽管在这种情况下，循环是无限的）
    for (auto &thread : client_threads) {
        thread.join();
    }

    // 结束文件保存线程
    {
        lock_guard<mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();
}


//#include <mutex>

// extern std::mutex agent2center_mtx; // 引用全局互斥锁

void processAgentData() {
    while (true) {
        Agent2CentorQueue &queue1 = Agent2CentorQueue::instance();

        if (!(queue1.center_read_queue.IsEmpty())) {
            student serialized_data = *(queue1.center_read_queue.Pop());
            Student student2;
            //student.deserialize(des_ub);
            student2.name = serialized_data.namee;
            student2.number =serialized_data.number;
            student2.grade =serialized_data.gradle;
            student2.hobby[0] = serialized_data.hobby[0];
            student2.hobby[1] = serialized_data.hobby[1];
            student2.hobby[2] = serialized_data.hobby[2];
            //std::lock_guard<std::mutex> lock(mtx);  // 锁定
            // 处理数据
            std::cout << "Processed Data - Name: " << student2.name << ", Number: " << student2.number
                      << ", Grade: " << student2.grade << ", Hobbies: " << student2.hobby[0] << ", "
                      << student2.hobby[1] << ", " << student2.hobby[2] << std::endl;

            // 将学生数据添加到全局 students 容器中
                {
                    //lock_guard<mutex> lock(mtx);
                    students.push_back(student2);
                    if (students.size() >= 1000) {
                        saveFlag = true;
                    }
                }
            // 如果 students 容器中的数据量超过阈值，设置 saveFlag 以触发保存

            cv.notify_one(); // 通知保存线程
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 避免空转
        }
    }
}
// std::mutex agent2center_mtx;

void processClientData() {
    while (true) {
        Center2AgentQueue &queue2 = Center2AgentQueue::instance();

    }
}
void *agent(void *arg)
{
    AgentMesg *agentMesg = (AgentMesg *)arg;
    eprosima::uxr::AgentInstance &agent_instance = agent_instance.getInstance();

    if (!agent_instance.create(agentMesg->argcAgent, agentMesg->argvAgent))
    {
        printf("error");
        return NULL;
    }
    agent_instance.run();

    return NULL;
}

int main(int argc, char** argv) {
    Center2AgentQueue::instance();
    Agent2CentorQueue::instance();

    AgentMesg *agentMesg = new AgentMesg;
    agentMesg->argcAgent = argc;
    agentMesg->argvAgent = argv;

    pthread_t tidAgent;
    pthread_create(&tidAgent, NULL, agent, agentMesg);
    pthread_detach(tidAgent);
    // 启动文件保存线程
    thread saverThread(fileSaver);

    // 启动数据接收线程
    thread receiverThread(receiveData);
    // // 启动处理 Agent2CentorQueue 数据的线程
    thread processThread(processAgentData);

    // thread processThread2(processClientData);

    // 启动数据接收线程
    


    receiverThread.join();
    processThread.join();
    //processThread2.join();
    saverThread.join();

    return 0;
}
