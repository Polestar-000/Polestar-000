﻿#include "ClientOP.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include "RequestFactory.h"
#include "RequestCodec.h"
#include "RsaCrypto.h"
#include "TcpSocket.h"
#include "RespondFactory.h"
#include "RespondCodec.h"
#include "Message.pb.h"
#include "Hash.h"
using namespace std;
using namespace Json;

ClientOP::ClientOP(string jsonFile)
{
	cout << "9999999999" << endl;
	// 解析json文件, 读文件 -> Value
	ifstream ifs(jsonFile);
	Reader r;
	Value root;
	r.parse(ifs, root);
	// 将root中的键值对value值取出
	m_info.serverID = root["ServerID"].asString();
	m_info.clientID = root["ClientID"].asString();
	m_info.ip = root["ServerIP"].asString();
	m_info.port = root["Port"].asInt();
	cout << m_info.ip << ", 端口:" << m_info.port << endl;
}

ClientOP::~ClientOP()
{
}

bool ClientOP::seckeyAgree()
{
	// 0. 生成密钥对, 将公钥字符串读出
	Cryptographic rsa;
	// 生成密钥对
	rsa.generateRsakey(1024);
	// 读公钥文件
	ifstream ifs("public.pem");
	stringstream str;
	str << ifs.rdbuf();
	// 1. 初始化序列化数据
	// 序列化的类对象 -> 工厂类创建
	RequestInfo reqInfo;
	reqInfo.clientID = m_info.clientID;
	reqInfo.serverID = m_info.serverID;
	reqInfo.cmd = 1;	// 秘钥协商
	reqInfo.data = str.str();	// 非对称加密的公钥
	Hash a(T_SHA1);
	a.addData(str.str());
	reqInfo.sign = rsa.rsaSign(a.result());	// 公钥的签名
	CodecFactory* factory = new RequestFactory(&reqInfo);
	Codec* c =  factory->createCodec();
	// 得到序列化之后的数据, 可以将其发送给服务器端
	string encstr = c->encodeMsg();
	// 释放资源
	delete factory;
	delete c;

	// 套接字通信, 当前是客户端, 连接服务器
	TcpSocket* tcp = new TcpSocket;
	// 连接服务器
	cout << m_info.ip << ", 端口:" << m_info.port << endl;
	int ret = tcp->connectToHost(m_info.ip, m_info.port);
	if (ret != 0)
	{
		cout << "连接服务器失败..." << endl;
		return false;
	}
	cout << "连接服务器成功..." << endl;
	// 发送序列化的数据
	tcp->sendMsg(encstr);
	// 等待服务器回复
	string msg = tcp->recvMsg();

	// 解析服务器数据 -> 解码(反序列化)
	// 数据还原到 RespondMsg
	factory = new RespondFactory(msg);
	c = factory->createCodec();
	RespondMsg* resData = (RespondMsg*)c->decodeMsg();
	// 判断状态
	if (!resData->status())
	{
		cout << "秘钥协商失败" << endl;
		return false;
	}
	// 将得到的密文解密
	string key = rsa.rsaPriKeyDecrypt(resData->data());
	cout << "对称加密的秘钥: " << key << endl;

	delete factory;
	delete c;
	delete tcp;

	return true;
}

void ClientOP::seckeyCheck()
{
}

void ClientOP::seckeyZhuXiao()
{
}
