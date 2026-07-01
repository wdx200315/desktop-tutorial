/**
 * @file cluster.h
 * @brief 双机热备与集群管理
 */
#ifndef SMARTSCHED_CLUSTER_H
#define SMARTSCHED_CLUSTER_H

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <memory>
#include <functional>

namespace smartsched {

// ==================== 节点状态 ====================

/**
 * @brief 集群节点状态
 */
enum class NodeState {
    Unknown = 0,     // 未知
    Starting = 1,    // 启动中
    Active = 2,      // 主节点（活跃）
    Standby = 3,    // 从节点（待机）
    Recovering = 4,  // 恢复中
    Failed = 5,      // 失败
    Shutdown = 6     // 已关闭
};

/**
 * @brief 集群节点信息
 */
struct ClusterNode {
    std::string node_id;
    std::string host;
    int port;
    NodeState state;
    int64_t last_heartbeat;
    int64_t start_time;
    int priority;       // 优先级，数字越大优先级越高
    bool is_master;     // 是否为主节点
    double load;        // 当前负载
    int connections;    // 当前连接数
    
    ClusterNode() 
        : port(8888)
        , state(NodeState::Unknown)
        , last_heartbeat(0)
        , start_time(0)
        , priority(100)
        , is_master(false)
        , load(0.0)
        , connections(0) {}
};

// ==================== 选举算法 ====================

/**
 * @brief 主从选举器 - Raft简化版
 */
class ElectionManager {
public:
    static ElectionManager& instance();
    
    // 节点注册
    void registerNode(const ClusterNode& node);
    void unregisterNode(const std::string& node_id);
    
    // 心跳
    void heartbeat(const std::string& node_id);
    
    // 选举
    bool startElection(const std::string& node_id);
    std::string getCurrentMaster() const;
    bool isMaster(const std::string& node_id) const;
    
    // 状态查询
    NodeState getNodeState(const std::string& node_id) const;
    std::vector<ClusterNode> getAllNodes() const;
    int getActiveNodeCount() const;
    
    // 选举超时配置
    void setElectionTimeout(int64_t ms);
    void setHeartbeatInterval(int64_t ms);
    
    // 回调
    using MasterChangeCallback = std::function<void(const std::string&, NodeState)>;
    void setOnMasterChange(MasterChangeCallback cb);

private:
    ElectionManager();
    void checkElection();
    bool checkQuorum() const;
    void electNewMaster();

private:
    std::map<std::string, ClusterNode> m_nodes;
    std::string m_currentMaster;
    std::atomic<bool> m_electionInProgress;
    int64_t m_electionTimeout;
    int64_t m_heartbeatInterval;
    int64_t m_lastElectionTime;
    MasterChangeCallback m_onMasterChange;
    mutable std::mutex m_mutex;
};

// ==================== 故障检测 ====================

/**
 * @brief 健康检查器
 */
class HealthChecker {
public:
    HealthChecker();
    ~HealthChecker();
    
    // 检查目标节点健康状态
    bool checkHealth(const std::string& host, int port);
    int checkAllNodes();
    
    // 配置
    void setCheckInterval(int64_t ms);
    void setTimeout(int64_t ms);
    void addExpectedPort(int port);
    void removeExpectedPort(int port);
    
    // 回调
    using HealthChangeCallback = std::function<void(const std::string&, bool)>;
    void setOnHealthChange(HealthChangeCallback cb);

private:
    int64_t m_checkInterval;
    int64_t m_timeout;
    std::vector<int> m_expectedPorts;
    std::map<std::string, bool> m_lastResults;
    std::thread m_thread;
    std::atomic<bool> m_running;
    HealthChangeCallback m_onHealthChange;
};

// ==================== 数据同步 ====================

/**
 * @brief 同步数据类型
 */
enum class SyncDataType {
    Registration,   // 挂号记录
    Queue,         // 队列数据
    Statistics,    // 统计数据
    Config,       // 配置变更
    User          // 用户数据
};

/**
 * @brief 同步任务
 */
struct SyncTask {
    int64_t task_id;
    SyncDataType data_type;
    std::string data;
    int64_t timestamp;
    std::string source_node;
    std::vector<std::string> target_nodes;
    int retry_count;
    bool completed;
};

/**
 * @brief 数据同步器
 */
class DataSyncer {
public:
    static DataSyncer& instance();
    
    // 同步操作
    void syncData(SyncDataType type, const std::string& data, 
                  const std::vector<std::string>& targets = {});
    void requestSync(const std::string& node_id, SyncDataType type);
    
    // 同步状态
    bool isSynced() const;
    double getSyncProgress() const;
    int64_t getLastSyncTime() const;
    
    // 冲突处理
    enum class ConflictStrategy {
        LatestWins,      // 最新数据胜出
        MasterWins,      // 主节点数据优先
        Merge,           // 合并
        Manual           // 手动解决
    };
    void setConflictStrategy(ConflictStrategy strategy);
    
    // 回调
    using SyncCompleteCallback = std::function<void(int64_t, bool)>;
    using ConflictCallback = std::function<std::string(const std::string&, const std::string&)>;
    
    void setOnSyncComplete(SyncCompleteCallback cb);
    void setOnConflict(ConflictCallback cb);

private:
    DataSyncer();
    void processSyncTasks();
    void handleSyncResponse(int64_t task_id, bool success);

private:
    std::vector<SyncTask> m_pendingTasks;
    std::map<int64_t, SyncTask> m_activeTasks;
    int64_t m_lastSyncTime;
    ConflictStrategy m_conflictStrategy;
    SyncCompleteCallback m_onSyncComplete;
    ConflictCallback m_onConflict;
    std::thread m_syncThread;
    std::atomic<bool> m_running;
    mutable std::mutex m_mutex;
};

// ==================== 连接池管理 ====================

/**
 * @brief 集群连接池
 */
class ClusterConnectionPool {
public:
    ClusterConnectionPool();
    ~ClusterConnectionPool();
    
    // 连接管理
    bool connect(const std::string& host, int port);
    void disconnect(const std::string& node_id);
    void disconnectAll();
    
    // 获取连接
    int getConnection(const std::string& node_id);
    std::vector<int> getAllConnections();
    int getActiveConnectionCount() const;
    
    // 负载均衡
    std::string getLeastLoadedNode();
    std::string getRandomNode();
    
    // 健康检查
    void markUnhealthy(const std::string& node_id);
    void markHealthy(const std::string& node_id);
    bool isHealthy(const std::string& node_id) const;
    
    // 统计
    struct PoolStats {
        int totalConnections;
        int healthyConnections;
        int unhealthyConnections;
        double averageLoad;
        std::map<std::string, int> connectionsByNode;
    };
    PoolStats getStats() const;

private:
    struct ConnectionInfo {
        int socket;
        std::string host;
        int port;
        bool healthy;
        double load;
        int64_t lastUsed;
    };
    
    std::map<std::string, ConnectionInfo> m_connections;
    mutable std::mutex m_mutex;
};

// ==================== 分布式锁 ====================

/**
 * @brief 分布式锁类型
 */
enum class LockType {
    ReadLock,   // 读锁
    WriteLock,  // 写锁
    Mutex       // 互斥锁
};

/**
 * @brief 分布式锁
 */
class DistributedLock {
public:
    DistributedLock(const std::string& name, LockType type, int64_t ttl_ms = 30000);
    ~DistributedLock();
    
    bool acquire();
    bool release();
    bool tryAcquire(int64_t timeout_ms);
    
    std::string getLockId() const { return m_lockId; }
    bool isAcquired() const { return m_acquired; }

private:
    std::string m_name;
    LockType m_type;
    std::string m_lockId;
    bool m_acquired;
    int64_t m_ttl;
    int64_t m_acquireTime;
};

/**
 * @brief 分布式锁管理器
 */
class LockManager {
public:
    static LockManager& instance();
    
    // 锁操作
    std::shared_ptr<DistributedLock> getLock(const std::string& name, LockType type);
    bool releaseLock(const std::string& lockId);
    
    // 锁查询
    bool isLocked(const std::string& name) const;
    std::string getLockHolder(const std::string& name) const;
    
    // 清理过期锁
    void cleanupExpiredLocks();

private:
    LockManager();
    
    std::string generateLockId(const std::string& name);
    bool extendLock(const std::string& lockId, int64_t ttl_ms);
    
    struct LockInfo {
        std::string lock_id;
        std::string holder_id;
        std::string name;
        LockType type;
        int64_t expires_at;
    };
    
    std::map<std::string, LockInfo> m_locks;
    mutable std::mutex m_mutex;
};

// ==================== 集群管理器 ====================

/**
 * @brief 集群管理器 - 单例
 */
class ClusterManager {
public:
    static ClusterManager& instance();
    
    // 初始化
    void init(const std::string& nodeId, const std::string& host, int port);
    void start();
    void stop();
    
    // 节点操作
    void joinCluster(const std::string& host, int port);
    void leaveCluster();
    
    // 状态
    NodeState getState() const { return m_state; }
    bool isMaster() const;
    std::string getMasterNodeId() const;
    
    // 故障转移
    void triggerFailover();
    bool isFailoverInProgress() const;
    
    // 统计
    struct ClusterStats {
        NodeState localState;
        std::string masterNodeId;
        int totalNodes;
        int healthyNodes;
        int64_t uptime;
        double syncProgress;
    };
    ClusterStats getStats() const;
    
    // 回调
    using StateChangeCallback = std::function<void(NodeState, NodeState)>;
    using FailoverCallback = std::function<void(const std::string&)>;
    
    void setOnStateChange(StateChangeCallback cb);
    void setOnFailover(FailoverCallback cb);

private:
    ClusterManager();
    void heartbeatLoop();
    void handleNodeFailure(const std::string& nodeId);
    void becomeMaster();
    void becomeStandby();

private:
    std::string m_nodeId;
    std::string m_host;
    int m_port;
    NodeState m_state;
    int64_t m_startTime;
    std::atomic<bool> m_running;
    std::thread m_heartbeatThread;
    
    StateChangeCallback m_onStateChange;
    FailoverCallback m_onFailover;
    
    ElectionManager* m_electionManager;
    HealthChecker* m_healthChecker;
    DataSyncer* m_dataSyncer;
    ClusterConnectionPool* m_connectionPool;
};

} // namespace smartsched

#endif // SMARTSCHED_CLUSTER_H
