
# LUA 接口文档

---------------------------------------------------------------------------------------

### agent接口

* agent.sendPktout(code, ...)
    有如下两种形式, 自动判断类型进行编码，或是指定采用指定类型
    1.  sendPktout(code, 1, 2, 'hello', 1.2, true) 将自动识别编码格式为 varInt, varInt, string, float, bool
    2.  sendPktout(code, '@@1=i,2=d,3=[name=s,age=i],4=s', 1, 2.2, {{age=12,name='alice'},{age=22,name='lucy'}}, 'hello') 将使用的指定的格式化方式进行编码
        以@@开头将被解析为格式字符串，其中类型含义如下, 
        b:  boolean
        i:  integer
        f:  float
        d:  double
        s:  string
j       []: table       可指定table中的属性名,仅会对指定的属性按顺序进行编码

* agent.replyPktout(session, code, ...)
    与`sendPktout`类型,但是需要指定session,在rpc调用中使用

* agent.exit()
    断开与客户端的连接

* agent.setTrust()
    设置为信任模式


---------------------------------------------------------------------------------------

### 输出协议

* pktout = agent.newPktout(code, size, session = 0)
    创建一个PacketOut

* pktout:write(...)
    写入数据，自动识别lua类型(浮点型为float)

* agent.send(pktout)
    发送pktout

---------------------------------------------------------------------------------------

### 读取协议

* pktin:code()
    获取协议号

* pktin:session()
    获取会话

* pktin:readInteger()
    读取varInt

* pktin:readString()
    读取string

* pktin:readBool()/pktin:readBoolean()
    读取bool

* pktin:readFloat()
    读取float

* pktin:readDouble()
    读取double

* pktin:read(format)
    按精简的格式字符串读取 'iissi' 将读取 varInt,varInt,string,string,varInt
    // (未实现) 按格式字符串读取 '@@1=s,2=i,3=[name=s,age=i]' 将读取string,varInt,{{name=string,age=varInt}} (`TODO`)

---------------------------------------------------------------------------------------

### 数据库
  local dbo = require('dbo')

* db = dbo.open(id)
    打开编号为id的数据库连接, 返回db对象

* rs = db:execute(sql)
    执行一条sql
    返回rs为一个table
    rs.ok 指示语句是否执行成功，不成功的话可以从rs.errorMessage获得错误信息
    rs.affectedRows 指示受影响的行数
    rs.rowsCount 指示数据总行数
    for _, row in ipairs(rs) do
        var_dump(row)
    end
    可对数据进行遍历操作

* rs = db:executePrepare(sql, ...)
    执行一条prepared sql
    例如 db:executePrepare('SELECT * FROM user WHERE uid=? and state=?', 12, 'normal')

* rs = db:insertBatch(tableName, dataList, updateFields)
    将数据批量插入到一个表中, updateFields可不传，表示如果主键存在时需要更新的列

* db:close()
    手动关闭，一般不需要手动调用

---------------------------------------------------------------------------------------

### HTTP
    local http = require('http')

#### HTTP Client  `需要在coroution中执行`

* code, str = http.getForString(url, params)
    以HTTP GET的方式请求指定url, 以字符串的形式返回获取的内容, 参数为table, key value将拼接为QueryString

* code, obj = http.getForObject(url, params)
    以HTTP GET的方式请求指定url, 获取的内容将被作为json进行解析,同时转为lua table

* code, str = http.posForString(url, params)
    以HTTP POST的方式请求指定url, 以字符串的形式返回获取的内容, 参数为table, 将被序列化为json，存入body中进行POST

* code, obj = http.postForObject(url, params)
    以HTTP POST的方式请求指定url, 获取的内容将被作为json进行解析,同时转为lua table

#### Http Server

* server = http.createServer(ip, port)
    创建一个Http服务, 侦定指定的ip, port

* server:setHandlers({
            ['/path1'] = function(req, resp) end,
            ['/'] = function(req, resp) end,
            ['/api/fetch'] = function(req, resp) end
        })
    添加处理函数, req详见`Http Request`, resp详见`Http Response`

* server:setResourceDir(dir)
    设置静态资源目录

#### Http Request
    req为一个lua table, 具体结构示例如下:

>   {
        method = 'GET',
        path = '/api/opt1',
        queries = {
            name = 'abc',
            value = '123'
        },
        headers = {
            'Cache-Control' = 'no-cache',
            'Host' = '127.0.0.1'
        },
        body = '{}'
    }

#### Http Response
    
* resp.headers = {name1 = 'value1', name2 = 'value2'}
    添加额外的响应头

* resp:send(content)
    发送内容

* resp:sendHtml(htmlContent)
    发送html, 将自动addHeader: Content-Type: text/html;charset=utf-8 

* resp:sendJson(jsonParams)
    发送json, 将自动addHeader: Content-Type: application/json;charset=utf-8

---------------------------------------------------------------------------------------

### 时间相关
    local timer = require('timer')

* local t = timer.setTimeout(callback, delay)
    延时执行一次, 单位:毫秒

* local t = timer.setInterval(callback, interval)
    重复执行

* t:cancel()
    手动取消定时器

* timer.getTickCache()
    获取当前时间, 单位:毫秒

* timer.getTimestampCache()
    获取当前时间, 单位:秒

* timer.sleep(interval)
    挂起当前协程，单位:毫秒 后唤醒

* timer.isTsShouldRefreshAtHour(timestamp, hour)
    某个时间是否需要刷新(小时为单位)

* timer.isTodayFromTimestamp(timestamp)
    指定时间戳是否当天

* timer.todayZeroTimestamp()
    获取当天0点的时间戳

---------------------------------------------------------------------------------------

### 集群相关
    local cluster = require('cluster')

##### event
    cluster.addNodeEventListener({onNodeUp = function(nodeid, name) end, onNodeDown = function(nodeid, name) end)
    添加节点事件监听

##### service
* local service, msg = cluster.createService('serviceName', serviceObj)
    创建一个服务

* serviceObj._onStubConnect(mbid)
    stub连接到service时触发 serviceObj的默认方法

* service:publish(mailboxId, ...) / service:publishToAll(...) / service:publishToAllExcept(mailboxId, ...)
    发布一条消息，指定订阅者将收到消息

* service:cast(mailboxId, ...)
    发送一条消息到指定邮箱

* service:from()
	消息来源

* service:session()
	当前调用的会话

* service:disableAutoReply()
    禁用自动回复call

* service:reply(session, ...)
	回应消息

* service:replyError(session, msg)
	回应错误

##### stub
* local stub, msg = cluster.connectService('serviceName')
    连接一个服务, 如果stub为nil则msg表示出错信息

* stub:method(...) / `stub:call_method(...)`
    调用服务, 返回 true, ... 或 false, message

* `stub:cast_message(...)`
    调用服务 （在不需要回应的情况下，效率更高)

* stub:setSubscriber(handler)
    设置订阅消息处理器, handler可以是function(...) 或 table
    
##### mailbox

* local mailbox, msg = cluster.createMailbox(handler)
    创建一个邮箱, handler可以是function(...) 或 table

* mailbox:getMailboxId()
    获取邮箱ID

* mailbox:cast(mailboxId, ...)
    发送消息至邮箱

* mailbox:delete()
    删除邮箱, 一般不需要手动删除，垃圾回收时将自动删除

---------------------------------------------------------------------------------------

### 框架

* framework.beforeShutdown(function(cb) end)
	注册一个函数，在服务器停止前调用, cb 回调时，再继续执行服务器停止操作

---------------------------------------------------------------------------------------

### 通用
    local utils = require('utils')

* utils.toJson(v)
    将lua值转为Json字符串

* utils.fromJson(str)
    将Json字符串转为lua值

* utils.sha1(plain)
    对plain进行SHA1加密并返回

* utils.md5(plain)
    对plain进行MD5加密并返回

* utils.base64Encode(plain)
    对plain进行base64编码并返回

* utils.base64Dncode(cipher)
    对cipher进行base64解码并返回
    
* utils.compress(data, level = 5)
    对data进行压缩并返回,level是压缩级别(范围1-9,缺省时为5)

* utils.uncompress(data)
    对data进行解压缩并返回

* utils.getResourceDir()
    获取资源目录

* utils.listFilesInDirectory(dir, depth = 1)
    获取dir目录中所有文件路径, 默认深度为1

* utils.countTable(table)
    遍历table,返回其大小

* utils.getRandomNum(max)
    生成随机数  0 <= 值 < max 

* utils.getRandomNum(min, max)
    生成随机数 min <= 值 < max 

* utils.serialize(val)
    序列化lua值, 返回用于存储的字符串,可以使用 dostring('return ' + str) 进行反序列化

* utils.createUid(serverId, uid)
    创建组合的用户ID = serverId左移32位或uid
* utils.getServerId(uid)
    从createUid已创建的uid提取出serverId
* utils.createItemId()
    创建一个唯一的数字物品ID
 * utils.getMapServiceId()
	
### 杂项(lua脚本通用接口,位于resource/script/misc.lua)
    local misc = require('libs/misc')
* misc.serialize(o, withoutKey)
    序列化一个TABLE到STRING

* misc.deserialize
    反序列化一个STRING到TABLE

---------------------------------------------------------------------------------------

### 过虑相关
    local trie = require('trie')

* trie.AddWord(word)
    加入一个脏字

* trie.isContain(text)
    检查文本是否包涵脏字

* local clean_data = trie.filter(data, rep = '*')
    过虑文本中的脏字并替换为指定符号，返回干净的文本

---------------------------------------------------------------------------------------
