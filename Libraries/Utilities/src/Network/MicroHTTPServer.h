//
// Created by marco.m_g on 15.07.2020.
//

#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <mutex>

#include "Network/Network_Common.h"
#include "Network/TCPListener.h"

namespace ara {

namespace microhttp {

class Server;

class Conn;

class Content {
public:
    bool        setName(std::string str);
    std::string getName();
    bool        setType(std::string str);
    std::string getType();
    bool        setFileName(std::string str);
    std::string getFileName();
    bool        setData(uint8_t *dataptr, int datasize);
    uint8_t    *getData();
    int         getSize();
    bool        storeToFile(std::filesystem::path &p);

private:
    std::string m_Name;
    std::string m_Type;
    std::string m_FileName;

    uint8_t *m_DataPtr  = nullptr;
    int      m_DataSize = 0;
};

class RawContent {
public:
    bool readContentData(Conn *conn, uint32_t count);
    bool readContentData(SOCKET sock, uint32_t count);

    void Reset() { m_Pos = 0; }

    int  MoveToNextBoundary();  // moves to the position after the boundary
    void SetBoundary(std::string &bstr);

    uint8_t *getData() { return (uint8_t *)m_ContentData.data(); }
    int      getSize() { return (int)m_ContentData.size(); }
    int      ReadLine(std::string &s, int max_size = 1024);

    int getCurrentPos() { return m_Pos; }

private:
    std::vector<uint8_t> m_ContentData;

    int m_Size    = 0;
    int m_Pos     = 0;
    int m_LastPos = 0;

    std::string m_Boundary;
};

class Conn {
public:
    using type_hdrmap  = std::map<std::string, std::string>;
    using type_request = std::vector<std::string>;
    using type_content = std::vector<std::shared_ptr<Content>>;

    Server *m_Server = nullptr;

    type_content m_Content;

    Conn(Server *m);

    bool     Process(SOCKET sock, sockaddr_in *addr);
    uint32_t getContentLength();

    std::string getHdrValue(std::string &name);

    std::string getHdrValue(char *name);

    std::string getMethod() { return m_Request.size() >= 1 ? m_Request[0] : ""; }
    std::string getURI() { return m_Request.size() >= 2 ? m_Request[1] : ""; }
    std::string getHTTP() { return m_Request.size() >= 3 ? m_Request[2] : ""; }

    std::string getQPar(std::string src, std::string parname);

    std::string getFromAddr() { return m_FromAddr; }

    int  SendString(std::string &s);
    int  SendError(int code);
    int  SendOK();
    int  SendResponse(int code, std::string &text_body);
    void ParseURI(std::string &filestr, std::vector<std::string> &parameter_list);
    int  sRecv(void *dest, int count);

private:
    RawContent m_RawContent;

    type_hdrmap  m_HdrMap;
    type_request m_Request;

    std::string m_FromAddr;

    SOCKET m_Socket;

    int64_t i_RecvByteCount = 0;

    void incRecvCount(int count);
};

class Server : public TCPListener {
public:
    using c_cb = std::function<void(Conn &conn)>;

    bool Start(int port, Server::c_cb const &f);
    void incRecvCount(int count);

    int64_t getRecvCount() { return i_RecvByteCount; }

private:
    std::mutex i_CMutex;

    c_cb m_OnConnection = nullptr;

    int64_t i_RecvByteCount = 0;

protected:
    virtual bool OnConnect(SOCKET sock, sockaddr_in *);
};

void utilParse(std::string &name, std::string &val, std::string &src_string, char delimiter = '=');

}  // namespace microhttp

}  // namespace ara
