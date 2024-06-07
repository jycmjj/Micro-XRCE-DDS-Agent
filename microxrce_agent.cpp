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

#define PORT 2024
using namespace std;
struct AgentMesg
{
    int argcAgent;
    char **argvAgent;
};

struct FileInfo {
    string filename;
    unsigned long long timestamp;
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

FileInfo saveToFile(const vector<Student>& students) {
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

    FileInfo fileInfo;
    fileInfo.filename = filename;
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
        }
    }
}

Student deserializeStudent(const char* buffer) { // 反序列化
    Student student;
    char name[50], hobby1[30], hobby2[30], hobby3[30];
    int fields = sscanf(buffer, "%29[^,],%ld,%ld,%29[^,],%29[^,],%29[^,]", name, &student.number, &student.grade, hobby1, hobby2, hobby3);
    student.name = string(name);
    student.hobby[0] = hobby1;
    student.hobby[1] = hobby2;
    student.hobby[2] = hobby3;
    return student;
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

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind 错误");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen 错误");
        exit(EXIT_FAILURE);
    }

    vector<thread> client_threads; 

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
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

    for (auto& thread : client_threads) {
        thread.join();
    }
    {
        lock_guard<mutex> lock(mtx);
        done = true;
    }
    cv.notify_one();
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


    receiverThread.join();
    saverThread.join();

    return 0;
}
