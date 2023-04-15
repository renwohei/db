/*
 * @Author: feng.lu
 * @Date: 2023-04-14 12:18:12
 * @Descripttion: --
 * @LastEditors: feng.lu
 * @LastEditTime: 2023-04-15 21:13:57
 */
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
using namespace std;

// const int N = 100000; // ���鳤��
// const int M = 10;     // �����߳���
// const int R = 10000;  // ÿ���߳��ظ�����

const int ARRs = 1000; // ���鳤��
const int WORKs = 100; // �����߳���
const int RUNs = 10000; // ÿ���߳��ظ�����

class Node {
public:
    unsigned long version; // �汾�ţ�ʹ��ʱ�������
    int data; // ����ֵ
    std::atomic<bool> lck; // �����λ
    std::atomic<bool> pin; // block���λ
    Node* next; // ָ����һ���ڵ�
    std::mutex mtx; // ������
public:
    Node()
        : version(get_scn())
        , data(0)
        , lck(false)
        , pin(false)
        , next(nullptr)
    {
    } // ���캯����ʹ�õ�ǰʱ�����Ϊ�汾��
    Node(int d, unsigned long version)
        : version(version)
        , data(d)
        , lck(false)
        , pin(false)
        , next(nullptr)
    {
    } // ���캯����ʹ�õ�ǰʱ�����Ϊ�汾��

    void to_string() const
    {
        std::cout << "version:" << version << ",data:" << data << std::endl;
    }
    void set_lck()
    {
        // lck = 1;
    }

    // ������ʼ���汾
    void set(int i)
    {
        this->data = i;
    }
    void set_version(unsigned long t_scn)
    {
        this->version = t_scn;
    }

    void set_data(int arr, int i)
    {
        int t_arr = arr;
        unsigned long t_scn = get_scn(); // ��ȡ������
        this->pin.store(true); // ģ��pin buffer���ڴ��޸Ĳ���

        Node* new_node = new Node(this->data, this->version);
        new_node->next = this->next;
        this->next = new_node;
        this->data = i;
        this->version = t_scn;
        // new_node->pin.store(false);
        //  new_node->next         = g_list;             // ���½ڵ�ָ��ɽڵ�
        //  g_list                 = new_node;           // ��ȫ��ָ��ָ���½ڵ�
        //  std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(100));
        // debug info
        // std::cout << "arr[" << t_arr << "] old data is:" << new_node->data << ",version:" << new_node->version << std::endl;
        // std::cout << "arr[" << t_arr << "] ###new value is-->" << this->data << ",version:" << this->version << std::endl;
        this->pin.store(false);
        // this->lck.store(false);
    }

    int get_data(unsigned long scn, int at_arr)
    {
        unsigned long t_scn = scn;
        int arr = at_arr;
        // ֻ���������,��д����ͻ;ʹ������ᵼ����mvccʱ��Ч�ʽϲ�
        while (this->pin) {
            // ����ȴ�buffer pin�ͷ�
            // std::cout << "wait pin" << std::endl;
        }
        // unsigned long t_scn = this->get_scn();
        Node* cur = this; // ��ǰָ��
        while (cur && cur->version > t_scn) { // ���������ҵ���Ӧʱ���Ľڵ�
            // std::cout << "read to next version" << endl;
            // std::cout << "array:[" << arr << "] scn>" << t_scn << " read to next version-->" << cur->version << ", data: " << cur->data << std::endl;
            cur = cur->next;
        }
        if (cur && cur->version <= t_scn) { // ����ҵ��ˣ��ͽ�ȫ��ָ��ָ��ýڵ�
            // std::cout << "array:[" << arr << "] scn>" << t_scn << "###get wanting data version###: " << cur->version << "  ----- data: " << cur->data << "  -----" << std::endl;

            // std::cout << "array:[" << arr << "] version:" << cur->version << "  value: " << cur->data << ";" << std::endl;

            return cur->data;
        } else { // ���û�ҵ����������ʾ��Ϣ
            std::cout << "ORA-01555: " << t_scn << std::endl;
            return -1;
        }
    }

    static unsigned long get_scn()
    {
        // ת��Ϊʱ�����΢�룩
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        return timestamp;
    }
};

std::array<Node, ARRs> nodes;
std::mutex g_mutex; // ��ʼ��������

unsigned long get_scn()
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return timestamp;
}

void dump()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    // unsigned long t_scn = scn;
    for (int i = 0; i < ARRs; i++) {
        std::cout << "array:[" << i << "] count is:<-->";

        // std::cout << "scn=" << nodes[i].version << " value:" << nodes[i].data << "";
        Node* cur = nodes[i].next;
        int j = 1;
        while (cur != nullptr) {

            // std::cout << "<-->scn=" << cur->version << "-value:" << cur->data;
            cur = cur->next;
            j++;
        }
        cout.flush();
        std::cout << j << std::endl;
    }
}

void worker(unsigned long t_scn)
{
    // std::cout << "" << endl;
    std::random_device rd; // �����������
    std::mt19937 gen(rd()); // α���������
    std::uniform_int_distribution<> dis(0, ARRs - 1); // ���ȷֲ�
    unsigned long scn = t_scn;
    for (int i = 0; i < RUNs; ++i) {
        int x = dis(gen); // �������x
        int y = dis(gen); // �������y
        std::lock_guard<std::mutex> lock(nodes[y].mtx); // ����,���ͬʱ�޸����ں˵���˯��

        // ���Ż�
        //  nodes[y].lck.store(true);                       // �����ݹ���ã�undoһ���Զ�ȡ���п���û��������������ύ�˻�����lock��Ϣ��ֻ���ж�
        //   std::cout << "node:[" << y << "] = node[" << x << "]+node[" << x + 1 << "]+node[" << x + 2 << "];" << endl;


        nodes[y].set_data(y, nodes[x].get_data(scn, x) + nodes[(x + 1) % ARRs].get_data(scn, (x + 1) % ARRs) + nodes[(x + 2) % ARRs].get_data(scn, (x + 2) % ARRs)); // ����array[y]
        // std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(100));
        //  ���Ż�
        //  nodes[y].lck.store(false);
    }
};

void get_system_time()
{
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    auto usec = chrono::duration_cast<chrono::microseconds>(now.time_since_epoch()).count() % 1000000;
    cout << put_time(localtime(&t), "%Y-%m-%d %H:%M:%S") << "." << setw(6) << setfill('0') << usec << endl;
}

int main()
{
    // ����4�����̺߳�2��д�̺߳�1�������߳�

    // std::vector<Node> nodes; // ��������
    // for (int i = 0; i < N; ++i) {
    //     nodes.emplace_back(i, Node::get_scn());
    // }
    // // for (const Node &node : nodes) {
    // //     node.to_string();
    // // }
    // std::cout << "vector count is:" << nodes.size() << endl;
    std::cout << "------------------------------------------------------" << endl;
    std::cout << "array is:1000, work thread 100, work thread loop 10000" << endl;
    std::cout << "------------------------------------------------------" << endl;
    get_system_time();
    for (int i = 0; i < ARRs; i++) {
        std::lock_guard<std::mutex> lock(g_mutex);
        nodes[i].data = i;
        nodes[i].version = 0;
        nodes[i].next = nullptr;
        nodes[i].pin = false;
        nodes[i].lck = false;
        // nodes[i].set_data(i);
        //  nodes[i].set_version(Node::get_scn());
    }
    // std::cout << "vector count is:" << nodes.size() << endl;
    //  std::cout << "vector count is:" << nodes[1000].data << endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < WORKs; ++i) {
        threads.push_back(std::thread { worker, get_scn() });
    }

    for (auto& t : threads) {
        t.join();
    }

    get_system_time();
    std::cout << "------------------------------------------------------" << endl;

    // for (int i = 0; i < ARRs; ++i) {
    //     std::lock_guard<std::mutex> lock(g_mutex);
    //     // std::cout << i << " is:" << nodes[i].get_data(get_scn(), i) << endl;
    //     std::cout << i << " is:" << nodes[i].data << endl;
    // }

    dump();

    // std::thread readers[RE];
    // std::thread writers[WR];

    // for (int i = 0; i < WR; i++) {
    //     writers[i] = std::thread{write_data, i, get_scn()};
    // }

    // for (int i = 0; i < RE; i++) {
    //     std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(50));
    //     readers[i] = std::thread{read_data, i, get_scn()};
    // }

    // // �ȴ������߳̽���
    // for (int i = 0; i < RE; i++) {
    //     readers[i].join();
    // }

    // for (int i = 0; i < WR; i++) {
    //     writers[i].join();
    // }

    return 0;
}