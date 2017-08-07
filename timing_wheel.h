/*
 * timing_wheel.h
 *
 *  Created on: 2017年8月01日
 *      Author: 高圣易
 */

#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <iostream>
#include <functional>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <set>
#include <thread>

/*定时器类*/
class tw_timer {
public:
	tw_timer(int rot,int ts) :next(NULL), prev(NULL), rotation(rot), timeSlot(ts) {}
	~tw_timer() {}
public:
	int rotation;  		//记录定时器在时间轮转多少圈后生效
	int timeSlot;      //记录定时器属于时间轮上的哪个槽(对应的链表，下同)
	std::function<void(const std::string&)> func; //定时器回调函数
	tw_timer *next;
	tw_timer *prev;
	std::string reqId;
};

class timing_wheel {
private:
	static const int N = 60; //时间轮上槽的数据
	static const int SI = 1; //每1s时间轮转动一次，即槽 间隔为1s
	tw_timer *m_slots[N]; //时间轮的槽，其中每个元素指向一个定时器链表，链表无序
	int m_curSlot; //时间轮的当前槽
	int m_timeout;  //超时间隔
	std::set<std::string> m_cancleTimer; //

public:
	timing_wheel(int timeout = 60) {
		m_curSlot = 0;
		m_timeout = timeout;

		for(int i = 0; i < N; i++) {
			m_slots[i] = NULL;  //初始化每个槽的头结点
		}
	}

	~timing_wheel() {
		//遍历整个槽
		for(int i = 0; i < N; i++) {
			tw_timer *tmp = m_slots[i];
			while(tmp) {
				m_slots[i] = tmp->next;
				delete tmp;
				tmp = m_slots[i];
			}
		}
	}

	//根据定时值timetout创建一个定时器，并插入它合适的槽中
	tw_timer* add_timer(const std::string& reqId, std::function<void(const std::string&)> func) {
		if(m_timeout < 0) {
			return NULL;
		}

		int ticks = 0;
		if(m_timeout < SI) {
			ticks = 1;
		} else {
			ticks = m_timeout / SI;
		}

		//计算待插入的定时器z在时间轮转动多少圈后被触发
		int rotation = ticks / N;

		//计算待插入的定时器应该被插入哪个槽中
		int ts = (m_curSlot + (ticks % N)) % N;

		//创建新的定时器, 他在时间轮转动rotation圈滞后被触发，且位于第ts个槽上
		tw_timer* timer = new tw_timer(rotation, ts);
		timer->func = func;
		timer->reqId = reqId;

		//如果ts个槽中尚无定时器，则把新建的定时器插入其中，并将该定时器设置为该槽的头结点
		if(!m_slots[ts]) {
			printf("add timer ,rotation is %d,ts is %d,m_curSlot is %d\n\n", rotation, ts, m_curSlot);
			m_slots[ts] = timer;
		} else {
            //否则,将定时器插入ts槽中
			timer->next = m_slots[ts];
			m_slots[ts]->prev = timer;
			m_slots[ts] = timer;
		}
	}

	//删除目标定时器
	void del_timer(const std::string& reqId) {
		m_cancleTimer.insert(reqId);
	}

	//SI 时间到后，调用该函数，时间轮向前滚动一个槽的间隔，使用方法，每隔一秒执行一次这函数
	void run() {
		std::thread t([this]() {
			while(true) {
				sleep(1);

				tw_timer *tmp = m_slots[m_curSlot];
				
				while(tmp) {
					bool isCancle = false;

					if (m_cancleTimer.size() > 0) {
						std::set<std::string>::iterator it = m_cancleTimer.find(tmp->reqId);
						if (it != m_cancleTimer.end()) {
							m_cancleTimer.erase(it);
							isCancle = true;
						}
					}

					//如果定时器的rotation值大于0，则他在这一轮不起作用
					if(tmp->rotation > 0) {
						tmp->rotation--;
						tmp = tmp->next;
					} else {
						//否则，说明定时器已经到期了，于是执行定时任务，然后删除定时器

						if (!isCancle) {
							tmp->func(tmp->reqId); //这里要改为协程执行
						}
						
						if(tmp == m_slots[m_curSlot]) {
							printf("delete header in m_curSlot\n");
							m_slots[m_curSlot] = tmp->next;
							delete tmp;
							if(m_slots[m_curSlot]) {
								m_slots[m_curSlot]->prev = NULL;
							}
							tmp = m_slots[m_curSlot];
						} else {
							tmp->prev->next = tmp->next;
							if(tmp->next) {
								tmp->next->prev = tmp->prev;
							}
							tw_timer *tmp2 = tmp->next;
							delete tmp;
							tmp = tmp2;
						}
					}
				}

				//更新当前时间轮的槽，以反应时间轮的转动
				m_curSlot = ++m_curSlot % N;
			}
        });
	
		t.detach();
	}
};

#endif