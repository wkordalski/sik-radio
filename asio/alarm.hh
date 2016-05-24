#pragma once

#include <map>

namespace asio {
    class Alarm {
    public:
        using clock = std::chrono::steady_clock;
        using task = unsigned long long int;

    protected:
        std::map<task, clock::time_point> tasks;
        std::map<std::pair<clock::time_point, task>, std::function<void()>> schedule;
        task counter = 0;

    public:
        task at(clock::time_point time, std::function<void()> action) {
            auto now = clock::now();
            if(now >= time) {
                action();
                return 0;
            } else {
                task current = ++counter;
                tasks[current] = time;
                schedule[{time, current}] = action;
                return current;
            }
        }

        void cancel(task t) {
            auto task_iterator = tasks.find(t);
            if(task_iterator == tasks.end()) return;
            auto time = task_iterator->second;
            tasks.erase(task_iterator);
            auto schedule_iterator = schedule.find({time, t});
            if(schedule_iterator != schedule.end()) {
                schedule.erase(schedule_iterator);
            }
        }

        clock::duration sleep_time(clock::duration maximum) {
            if(tasks.empty()) {
                return maximum;
            }

            auto now = clock::now();
            auto element = schedule.begin();
            auto run_time = element->first.first;
            return std::min(run_time - now, maximum);
        }

        void refresh() {
            auto now = clock::now();
            while(true) {
                auto element = schedule.begin();
                if(element == schedule.end()) {
                    return;
                }
                auto run_time = element->first.first;
                task tid = element->first.second;
                if(run_time <= now) {
                    element->second();
                    schedule.erase(element);
                    tasks.erase(tid);
                } else {
                    return;
                }
            }
        }

        bool empty() { return schedule.empty(); }
    };
}
