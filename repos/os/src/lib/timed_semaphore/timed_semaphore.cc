/*
 * \brief  Semaphore implementation with timeout facility.
 * \author Stefan Kalkowski
 * \date   2010-03-05
 */

/*
 * Copyright (C) 2010-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#define INCLUDED_FROM_TIMED_SEMAPHORE_CC
#include <os/timed_semaphore.h>

using namespace Genode;


/**
 * Alarm thread, which counts jiffies and triggers timeout events.
 */
class Timeout_thread : public Thread, public Alarm_scheduler
{
	private:

		enum { JIFFIES_STEP_MS = 10 };

		Timer::Connection _timer;    /* timer session   */
		Signal_context    _context;
		Signal_receiver   _receiver;

		void entry(void);

	public:

		Timeout_thread(Env &env)
		:
			Thread(env, "alarm-timer", 2048*sizeof(long)), _timer(env)
		{
			_timer.sigh(_receiver.manage(&_context));
			_timer.trigger_periodic(JIFFIES_STEP_MS*1000);
			start();
		}

		Alarm::Time time(void) { return _timer.elapsed_ms(); }

		/*
		 * Returns the singleton timeout-thread used for all timeouts.
		 */
		static Timeout_thread &alarm_timer();
};


Timed_semaphore::Timeout::Timeout(Time duration,
                                  Timed_semaphore *s, Element *e)
:
	_sem(s), _element(e), _triggered(false)
{
	Timeout_thread &tt = Timeout_thread::alarm_timer();
	_start = tt.time();
	tt.schedule_absolute(this, _start + duration);
}


void Timed_semaphore::Timeout::discard()
{
	Timeout_thread::alarm_timer().discard(this);
}


Alarm::Time Timed_semaphore::down(Alarm::Time t)
{
	Semaphore::_meta_lock.lock();

	if (--Semaphore::_cnt < 0) {

		/* If t==0 we shall not block */
		if (t == 0) {
			++_cnt;
			Semaphore::_meta_lock.unlock();
			throw Nonblocking_exception();
		}

		/*
		 * Create semaphore queue element representing the thread
		 * in the wait queue.
		 */
		Element queue_element;
		Semaphore::_queue.enqueue(&queue_element);
		Semaphore::_meta_lock.unlock();

		/* Create the timeout */
		Timeout to(t, this, &queue_element);

		/*
		 * The thread is going to block on a local lock now,
		 * waiting for getting waked from another thread
		 * calling 'up()'
		 * */
		queue_element.block();

		/* Deactivate timeout */
		to.discard();

		/*
		 * When we were only woken up, because of a timeout,
		 * throw an exception.
		 */
		if (to.triggered())
			throw Timeout_exception();

		/* return blocking time */
		return Timeout_thread::alarm_timer().time() - to.start();
	} else {
		Semaphore::_meta_lock.unlock();
	}
	return 0;
}


void Timeout_thread::entry()
{
	while (true) {
		Signal s = _receiver.wait_for_signal();

		/* handle timouts of this point in time */
		Alarm_scheduler::handle(_timer.elapsed_ms());
	}
}


static Env *_env_ptr;


Timeout_thread &Timeout_thread::alarm_timer()
{
	if (!_env_ptr) {
		error("missing call of Timed_semaphore::init");
		struct Missing_timed_semaphore_init { };
		throw  Missing_timed_semaphore_init();
	}
	static Timeout_thread inst(*_env_ptr);
	return inst;
}


void Timed_semaphore::init(Env &env) { _env_ptr = &env; }
