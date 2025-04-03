#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "http/HttpUtil.h"
#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "mysql/MysqlConnPool.h"
#include "jsoncpp/json/json.h"
#include "log/Logger.h"
#include <signal.h>
#include <thread>
#include <mutex>
#include"until.hpp"
#include <condition_variable>

void handleLoginRequest(const HttpRequest &req, HttpResponse *resp);
void HandleRegisterRequest(const HttpRequest &req, HttpResponse *resp);
void HandleLogoutRequest(const HttpRequest &req, HttpResponse *resp); // 新增
void handleGetChatRooms(const HttpRequest &req, HttpResponse *resp);
void handleGetMessages(const HttpRequest &req, HttpResponse *resp);