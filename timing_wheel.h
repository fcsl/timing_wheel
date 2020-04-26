#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <iostream>
#include <functional>
#include <time.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>
#include <set>
#include <thread>

class timing_wheel {
private:
  int select_sleep(int ms) {
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = ms % 1000 * 1000;

    while (true) {
      if ((tv.tv_sec == 0) && (tv.tv_usec == 0)) {
        break;
      }
      select(0, NULL, NULL, NULL, &tv);
    }
  }

  /*定时器类*/
  class tw_timer {
  public:
    tw_timer(int rot, int interval)
        : next(NULL), prev(NULL), m_rotation(rot), m_interval(interval) {}
    ~tw_timer() {}

  public:
    int m_rotation; //记录定时器在时间轮转多少圈后生效
    int m_interval; //记录定时器定时间隔
    std::function<void(const std::string &)> func; //定时器回调函数
    tw_timer *next;
    tw_timer *prev;
    std::string reqId;
  };

private:
  static const int N = 100; //时间轮上槽的数据
  tw_timer *m_slots[N]; //时间轮的槽，其中每个元素指向一个定时器链表
  int m_curSlot;                       //时间轮的当前槽
  int m_granularity;                   //最小时间精度
  std::set<std::string> m_cancleSets; //待取消定时器集合

public:
  timing_wheel(int granularity) {
    m_curSlot = 0;
    m_granularity = granularity;

    for (int i = 0; i < N; i++) {
      m_slots[i] = NULL;
    }
  }

  ~timing_wheel() {
    for (int i = 0; i < N; i++) {
      tw_timer *tmp = m_slots[i];
      while (tmp) {
        m_slots[i] = tmp->next;
        delete tmp;
        tmp = m_slots[i];
      }
    }
  }

  //根据定时值interval创建一个定时器，并插入它合适的槽中
  tw_timer *add_timer(const std::string &reqId, bool isfirst, int interval,
                      std::function<void(const std::string &)> func) {
    if (interval < 0) {
      return NULL;
    }

    int ticks = 0;
    if (interval < m_granularity) {
      ticks = 1;
    } else {
      ticks = interval / m_granularity;
    }

    //计算待插入的定时器在时间轮转动多少圈后被触发
    int rotation = ticks / N;
    if (isfirst == false) {
      --rotation;
    }

    //计算待插入的定时器应该被插入哪个槽中
    int ts = (m_curSlot + (ticks % N)) % N;

    //创建新的定时器, 他在时间轮转动rotation圈滞后被触发，且位于第ts个槽上
    tw_timer *timer = new tw_timer(rotation, interval);
    timer->func = func;
    timer->reqId = reqId;
    timer->m_interval = interval;

    //如果ts个槽中尚无定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头结点
    if (!m_slots[ts]) {
      printf("add timer ,rotation is %d,ts is %d,m_curSlot is %d\n\n", rotation,
             ts, m_curSlot);
      m_slots[ts] = timer;
    } else { //否则,将定时器插入ts槽中
      printf("add timer ,rotation is %d,ts is %d,m_curSlot is %d\n\n", rotation,
             ts, m_curSlot);
      timer->next = m_slots[ts];
      m_slots[ts]->prev = timer;
      m_slots[ts] = timer;
    }
  }

  //根据reqId删除目标定时器(先标记，等待轮转到目标槽后再真正删除timer节点)
  void del_timer(const std::string &reqId) { m_cancleSets.insert(reqId); }

  //根据链表节点地址删除目标定时器
  void del_timer(tw_timer *&tmp) {
    if (tmp == m_slots[m_curSlot]) {
      m_slots[m_curSlot] = tmp->next;
      delete tmp;
      if (m_slots[m_curSlot]) {
        m_slots[m_curSlot]->prev = NULL;
      }
      tmp = m_slots[m_curSlot];
    } else {
      tmp->prev->next = tmp->next;
      if (tmp->next) {
        tmp->next->prev = tmp->prev;
      }
      tw_timer *tmp2 = tmp->next;
      delete tmp;
      tmp = tmp2;
    }
  }

  //时间轮执行器
  void run() {
    std::thread t([this]() {
      while (true) {
        select_sleep(m_granularity);

        tw_timer *tmp = m_slots[m_curSlot];
        while (tmp) {
          bool isCancle = false;
          if (m_cancleSets.size() > 0) {
            std::set<std::string>::iterator it = m_cancleSets.find(tmp->reqId);
            if (it != m_cancleSets.end()) {
              m_cancleSets.erase(it);
              isCancle = true;
            }
          }

          //如果定时器的rotation值大于0，则他在这一轮不起作用
          if (tmp->m_rotation > 0) {
            tmp->m_rotation--;
            tmp = tmp->next;
          } else {
            if (!isCancle) {
              tmp->func(tmp->reqId);
            }

            int next_interval;
            std::function<void(const std::string &)> next_func;
            std::string next_reqId;
            next_interval = tmp->m_interval;
            next_func = tmp->func;
            next_reqId = tmp->reqId;

            //删除已经执行过的timer节点
            del_timer(tmp);

            if (!isCancle) {
              //重新添加下一个timer节点
              add_timer(next_reqId, false, next_interval, next_func);
            }
          }
        }

        //更新当前时间轮的槽
        m_curSlot = ++m_curSlot % N;
      }
    });

    t.detach();
  }
};

#endif
