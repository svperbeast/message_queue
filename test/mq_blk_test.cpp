#include "message_queue.h"
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <thread>
#include <chrono>

namespace {
    volatile sig_atomic_t sflag = 0;

    std::mutex io_mut_;
}

void signal_handler(int signo)
{
    sflag = 1;
}

struct Data {
    int sequence;
    int value;
};

static void consumer_routine(MessageQueue<Data>* dq)
{
    using namespace std;

    Data d;

    while (dq->Receive(d)) {
        {
            std::lock_guard<std::mutex> lk(io_mut_);
            std::cout << "id=" << std::this_thread::get_id() <<
                " received sequence=" << d.sequence <<
                " value=" << d.value << "\n";
        }
        if (d.value == 1)
        {
            break;
        }
    }
}

static void producer_routine(MessageQueue<Data>* dq)
{
    using namespace std;

    int sequence = 0;

    while (true) {
        Data d;
        d.sequence = sequence++;
        d.value = sflag;
        dq->Send(d);
        {
            std::lock_guard<std::mutex> lk(io_mut_);
            std::cout << "id=" << std::this_thread::get_id() <<
                " send sequence=" << d.sequence <<
                " value=" << d.value << "\n";
        }
        if (sflag == 1)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[])
{
    using namespace std;

    std::signal(SIGINT, signal_handler);

    MessageQueue<Data> dataQueue;

    std::thread consumer1(consumer_routine, &dataQueue);
    std::thread consumer2(consumer_routine, &dataQueue);
    std::thread producer1(producer_routine, &dataQueue);
    std::thread producer2(producer_routine, &dataQueue);

    while (!sflag) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    consumer1.join();
    consumer2.join();
    producer1.join();
    producer2.join();
    return 0;
}
