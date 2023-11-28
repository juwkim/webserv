#include "simple.hpp"

const default_t Simple::Default = default_s();

Simple::~Simple() { }
Simple::Simple() { }
Simple::Simple(const Simple &obj) { *this = obj; }
Simple &Simple::operator=(const Simple &obj)
{
    if (this == &obj)
        return *this;
    return *this;
}

default_s::default_s():
    listen(),
    ret(),
    keepalive_timeout(),
    try_files(),
    fastcgi_pass(),
    sendfile(false),
    autoindex(false),
    absolute_redirect(true),
    server_name_in_redirect(false),
    tcp_nodelay(true),
    tcp_nopush(false),
    default_type("text/html"),
    root("html"),
    server_name(""),
    send_lowat(0),
    sendfile_max_chunk(2 * 1024 * 1024),
    client_max_body_size(1 * 1024 * 1024),
    client_body_timeout(60),
    client_header_timeout(60),
    client_header_buffer_size(1 * 1024),
    keepalive_requests(1000),
    keepalive_time(1 * 60 * 60)
{
    linger.l_onoff = 1;
    linger.l_linger = 5;

    listen.address = "*";
    listen.port = "8000";

    index.push_back("index.html");
}

listen_s::listen_s():
    address(""),
    port(""),
    default_server(false),
    ssl(false),
    http2(false),
    quic(false),
    proxy_protocol(false),
    deferred(false),
    bind(false),
    ipv6only(false),
    reuseport(false),
    so_keepalive(false),
    keepidle(-1),
    keepintvl(-1),
    keepcnt(-1),
    setfib(-1),
    fastopen(-1),
    backlog(-1),
    rcvbuf(-1),
    sndbuf(-1),
    accept_filter(""),
    unixpath("")
{
}

return_s::return_s():
    code(0),
    url(""),
    text("")
{
}

keepalive_timeout_s::keepalive_timeout_s():
    timeout(75),
    header_timeout(-1)
{
}

try_files_s::try_files_s():
    files(),
    uri(""),
    code(0)
{
}

fastcgi_pass_s::fastcgi_pass_s():
    address(""),
    port(""),
    unix(false)
{
}

bool Simple::ParseBool(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg, const std::string &directive)
{
    bool res = false;
    if (idx == tokens.size()) {
        error_msg =  directive + ": Missing argument";
        return res;
    }
    if (tokens[idx] == "on")       res = true;
    else if (tokens[idx] == "off") res = false;
    else {
        error_msg = directive + ": Invalid argument [ " + tokens[idx] + " ]";
        return res;
    }
    ++idx;
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = directive + ": Missing ;";
    else
        ++idx;
    return res;
}

std::string Simple::ParseString(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg, const std::string &directive)
{
    std::string res;
    if (idx == tokens.size()) {
        error_msg = directive + ": Missing argument";
        return res;
    }
    res = tokens[idx++];
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = directive + ": Missing ;";
    else
        ++idx;
    return res;
}

keepalive_timeout_t Simple::ParseKeepaliveTimeout(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    keepalive_timeout_t keepalive_timeout;
    if (idx == tokens.size()) {
        error_msg =  "keepalive_timeout: Missing argument";
        return keepalive_timeout;
    }
    keepalive_timeout.timeout = Utils::StringtoTime(tokens[idx], error_msg);
    if (error_msg.empty() == false)
        return keepalive_timeout;
    ++idx;
    if (idx == tokens.size()) {
        error_msg = "keepalive_timeout: Missing ;";
        return keepalive_timeout;
    }
    if (tokens[idx] == ";") {
        ++idx;
        return keepalive_timeout;
    }
    keepalive_timeout.header_timeout = Utils::StringtoTime(tokens[idx], error_msg);
    if (error_msg.empty() == false)
        return keepalive_timeout;
    ++idx;
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = "keepalive_timeout: Missing ;";
    else
        ++idx;
    return keepalive_timeout;
}

listen_t Simple::ParseListen(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    listen_t listen;
    if (idx == tokens.size()) {
        error_msg =  "listen: Missing argument";
        return listen;
    }
    const std::string argument = tokens[idx];
    size_t pos = argument.find(":");

    if (pos == std::string::npos) {
        if (argument.find_first_not_of("0123456789") == std::string::npos)
            listen.port = argument;
        else {
            listen.address = argument;
            listen.port = "80";
        }
    } else if (argument.substr(0, pos) == "unix") {
        if (pos + 1 == argument.size()) {
            error_msg = "listen: Missing unix path";
            return listen;
        }
        listen.unixpath = argument.substr(pos + 1);
    } else {
        listen.address = argument.substr(0, pos);
        if (pos + 1 == argument.size()) {
            error_msg = "listen: Missing port";
            return listen;
        }
        const std::string port_s = argument.substr(pos + 1);
        if (port_s.find_first_not_of("0123456789") == std::string::npos) {
            listen.port = port_s;
        } else {
            error_msg = "listen: Invalid port";
            return listen;
        }
    }
    ++idx;

    while (idx < tokens.size() && tokens[idx] != ";") {
        const std::string option = tokens[idx];
        if (option == "default_server")
            listen.default_server = true;
        else if (option == "ssl")
            listen.ssl = true;
        else if (option == "http2") {
            if (listen.quic == true) {
                error_msg = "listen: http2: http2 and quic are mutually exclusive";
                return listen;
            }
            listen.http2 = true;
        }
        else if (option == "quic") {
            if (listen.http2 == true) {
                error_msg = "listen: quic: http2 and quic are mutually exclusive";
                return listen;
            }
            listen.quic = true;
        }
        else if (option == "proxy_protocol")
            listen.proxy_protocol = true;
        else if (option == "deferred")
            listen.deferred = true;
        else if (option == "bind")
            listen.bind = true;
        else if (option == "ipv6only")
            listen.ipv6only = true;
        else if (option == "reuseport")
            listen.reuseport = true;
        else if (option.substr(0, 12) == "so_keepalive") {
            if (option.size() < 14 || option[12] != '=') {
                error_msg = "listen: Missing so_keepalive value";
                return listen;
            }
            const std::string value = option.substr(13);
            if (value == "on")
                listen.so_keepalive = true;
            else if (value == "off")
                listen.so_keepalive = false;
            else {
                const size_t pos1 = value.find_first_of(":");
                const size_t pos2 = value.find_last_of(":");
                if (pos1 == std::string::npos || pos1 == pos2) {
                    error_msg = "listen: so_keepalive: Invalid value";
                    return listen;
                }
                listen.keepidle = Utils::StringtoTime(value.substr(0, pos1), error_msg);
                if (error_msg.empty() == false)
                    return listen;
                listen.keepintvl = Utils::StringtoTime(value.substr(pos1 + 1, pos2 - pos1 - 1), error_msg);
                if (error_msg.empty() == false)
                    return listen;
                listen.keepcnt = Utils::StringtoTime(value.substr(pos2 + 1), error_msg);
                if (error_msg.empty() == false)
                    return listen;
            }
        }
        else if (option.substr(0, 6) == "setfib") {
            if (option.size() < 8 || option[6] != '=') {
                error_msg = "listen: setfib: Missing number";
                return listen;
            }
            listen.setfib = Utils::StringtoSize(option.substr(7), error_msg);
            if (error_msg.empty() == false)
                return listen;
        } else if (option.substr(0, 8) == "fastopen") {
            if (option.size() < 10 || option[8] != '=') {
                error_msg = "listen: fastopen: Missing number";
                return listen;
            }
            listen.fastopen = Utils::StringtoSize(option.substr(9), error_msg);
            if (error_msg.empty() == false)
                return listen;
        } else if (option.substr(0, 7) == "backlog") {
            if (option.size() < 9 || option[7] != '=') {
                error_msg = "listen: backlog: Missing number";
                return listen;
            }
            listen.backlog = Utils::StringtoSize(option.substr(8), error_msg);
            if (error_msg.empty() == false)
                return listen;
        } else if (option.substr(0, 6) == "rcvbuf") {
            if (option.size() < 8 || option[6] != '=') {
                error_msg = "listen: rcvbuf: Missing number";
                return listen;
            }
            listen.rcvbuf = Utils::StringtoSize(option.substr(7), error_msg);
            if (error_msg.empty() == false)
                return listen;
        } else if (option.substr(0, 6) == "sndbuf") {
            if (option.size() < 8 || option[6] != '=') {
                error_msg = "listen: sndbuf: Missing number";
                return listen;
            }
            listen.sndbuf = Utils::StringtoSize(option.substr(7), error_msg);
            if (error_msg.empty() == false)
                return listen;
        } else if (option.substr(0, 13) == "accept_filter") {
            if (option.size() < 15 || option[13] != '=') {
                error_msg = "listen: accept_filter: Missing filter value";
                return listen;
            }
            listen.accept_filter = option.substr(14);
            if (listen.accept_filter != "dataready" && listen.accept_filter != "httpready") {
                error_msg = "listen: accept_filter: Invalid filter value";
                return listen;
            }
        } else {
            error_msg = "listen: Unknown option [ " + option + " ]";
            return listen;
        }
        ++idx;
    }
    if (idx == tokens.size())
        error_msg = "listen: Missing ;";
    else
        ++idx;
    return listen;
}

std::vector<std::string> Simple::ParseStringVector(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg, const std::string &directive)
{
    std::vector<std::string> res;
    if (idx == tokens.size()) {
        error_msg =  directive + ": Missing argument";
        return res;
    }
    while (idx < tokens.size() && tokens[idx] != ";")
        res.push_back(tokens[idx++]);
    if (idx == tokens.size())
        error_msg = directive + ": Missing ;";
    else
        ++idx;
    return res;
}

int Simple::ParseInt(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg, const std::string &directive, const bool is_time)
{
    int res = -1;
    if (idx == tokens.size()) {
        error_msg = directive + ": Missing argument";
        return res;
    }
    if (is_time)
        res = Utils::StringtoTime(tokens[idx], error_msg);
    else
        res = Utils::StringtoSize(tokens[idx], error_msg);
    if (error_msg.empty() == false)
        return res;
    ++idx;
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = directive + ": Missing ;";
    else
        ++idx;
    return res;
}

std::map<int, std::string> Simple::ParseErrorPage(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    std::map<int, std::string> error_page;

    if (idx == tokens.size()) {
        error_msg = "error_page: Missing status code";
        return error_page;
    }
    if (idx + 1 == tokens.size()) {
        error_msg = "error_page: Missing uri";
        return error_page;
    }

    std::vector<std::string> arguments;

    while (idx < tokens.size() && tokens[idx] != ";")
        arguments.push_back(tokens[idx++]);
    if (idx == tokens.size()) {
        error_msg = "error_page: Missing ;";
        return error_page;
    }
    else
        ++idx;

    std::string uri = arguments.back(); arguments.pop_back();
    for (size_t i = 0; i < arguments.size(); ++i) {
        int status_code = Utils::StringtoSize(arguments[i], error_msg);
        if (error_msg.empty() == false)
            return error_page;
        if (status_code < 100 || status_code > 599) {
            error_msg = "error_page: status code: Invalid range";
            return error_page;
        }
        if (error_page.find(status_code) != error_page.end()) {
            error_msg = "error_page: status code: Duplicate";
            return error_page;
        }
        error_page[status_code] = uri;
    }
    return error_page;
}

try_files_t Simple::ParseTryFiles(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    try_files_t try_files;

    if (idx == tokens.size()) {
        error_msg = "try_files: Missing file";
        return try_files;
    }
    if (idx + 1 == tokens.size()) {
        error_msg = "try_files: Missing uri or code";
        return try_files;
    }

    std::vector<std::string> arguments;

    while (idx < tokens.size() && tokens[idx] != ";")
        arguments.push_back(tokens[idx++]);
    if (idx == tokens.size()) {
        error_msg = "try_files: Missing ;";
        return try_files;
    }
    else
        ++idx;

    std::string token = arguments.back(); arguments.pop_back();

    if (token[0] == '=') {
        try_files.code = Utils::StringtoSize(token.substr(1), error_msg);
        if (error_msg.empty() == false)
            return try_files;
        if (try_files.code < 100 || try_files.code > 599) {
            error_msg = "try_files: code: Invalid range";
            return try_files;
        }
    } else
        try_files.uri = token;
    try_files.files = arguments;
    return try_files;
}

void Simple::ParseFastcgiParam(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg, std::map<std::string, std::string> &fastcgi_param)
{
    if (idx == tokens.size()) {
        error_msg = "fastcgi_param: Missing parameter";
        return;
    }
    if (idx + 1 == tokens.size()) {
        error_msg = "fastcgi_param: Missing value";
        return;
    }
    if (idx + 2 == tokens.size() || tokens[idx + 2] != ";") {
        error_msg = "fastcgi_param: Missing ;";
        return;
    }
    const std::string parameter = tokens[idx++];
    const std::string value = tokens[idx++];
    fastcgi_param[parameter] = value;
    ++idx;
    return;
}

fastcgi_pass_t Simple::ParseFastcgiPass(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    fastcgi_pass_t fastcgi_pass;
    if (idx == tokens.size()) {
        error_msg = "fastcgi_pass: Missing argument";
        return fastcgi_pass;
    }
    const std::string argument = tokens[idx++];
    size_t pos = argument.find_first_of(":");
    if (pos == std::string::npos) {
        fastcgi_pass.address = argument;
        fastcgi_pass.port = "8000";
    } else {
        fastcgi_pass.address = argument.substr(0, pos);
        fastcgi_pass.port = Utils::StringtoSize(argument.substr(pos + 1), error_msg);
        if (error_msg.empty() == false)
            return fastcgi_pass;
    }
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = "fastcgi_pass: Missing ;";
    else
        ++idx;
    return fastcgi_pass;
}

return_t Simple::ParseReturn(const std::vector<std::string> &tokens, size_t &idx, std::string &error_msg)
{
    return_t ret;
    if (idx == tokens.size()) {
        error_msg = "return: Missing code or url";
        return ret;
    }
    const std::string argument = tokens[idx++];
    if (std::isdigit(argument[0])) { // return code [text] or return code URL
        ret.code = Utils::StringtoSize(argument, error_msg);
        if (error_msg.empty() == false)
            return ret;
        if (ret.code < 100 || ret.code > 599) {
            error_msg = "return: code: Invalid range";
            return ret;
        }
        if (ret.code == 301 || ret.code == 302 || ret.code == 303 || ret.code == 307 || ret.code == 308) { // return code URL
            if (idx == tokens.size()) {
                error_msg = "return: Missing URL";
                return ret;
            }
            ret.url = tokens[idx++];
        } else { // return code [text]
            if (idx != tokens.size() && tokens[idx] != ";")
                ret.text = tokens[idx++];
        }
    } else // return URL
        ret.url = argument;
    if (idx == tokens.size() || tokens[idx] != ";")
        error_msg = "return: Missing ;";
    else
        ++idx;
    return ret;
}