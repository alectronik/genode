/*
 * \brief  Semaphore implementation with timeout facility
 * \author Stefan Kalkowski
 * \date   2010-03-05
 *
 * This semaphore implementation allows to block on a semaphore for a
 * given time instead of blocking indefinetely.
 *
 * For the timeout functionality the alarm framework is used.
 */

/*
 * Copyright (C) 2010-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__OS__TIMED_SEMAPHORE_H_
#define _INCLUDE__OS__TIMED_SEMAPHORE_H_

#include <base/thread.h>
#include <base/semaphore.h>
#include <timer_session/connection.h>
#include <os/alarm.h>

#ifndef INCLUDED_FROM_TIMED_SEMAPHORE_CC
#warning os/timed_semaphore.h is deprecated, use component API instead
#endif

namespace Genode {

	class Timed_semaphore;

	/**
	 * Exception types
	 */
	class Timeout_exception;
	class Nonblocking_exception;
}


class Genode::Timeout_exception     : public Exception { };
class Genode::Nonblocking_exception : public Exception { };


/**
 * Semaphore with timeout on down operation.
 */
class Genode::Timed_semaphore : public Semaphore
{
	private:

		typedef Semaphore::Element Element;

		/**
		 * Aborts blocking on the semaphore, raised when a timeout occured.
		 *
		 * \param  element the waiting-queue element associated with a timeout.
         * \return true if a thread was aborted/woken up
		 */
		bool _abort(Element *element)
		{
			Lock::Guard lock_guard(Semaphore::_meta_lock);

			/* potentially, the queue is empty */
			if (++Semaphore::_cnt <= 0) {

				/*
				 * Iterate through the queue and find the thread,
				 * with the corresponding timeout.
				 */
				Element *first = Semaphore::_queue.dequeue();
				Element *e     = first;

				while (true) {

					/*
					 * Wakeup the thread.
					 */
					if (element == e) {
						e->wake_up();
						return true;
					}

					/*
					 * Noninvolved threads are enqueued again.
					 */
					Semaphore::_queue.enqueue(e);
					e = Semaphore::_queue.dequeue();

					/*
					 * Maybe, the alarm was triggered just after the corresponding
					 * thread was already dequeued, that's why we have to track
					 * whether we processed the whole queue.
					 */
					if (e == first)
						break;
				}
			}

			/* The right element was not found, so decrease counter again */
			--Semaphore::_cnt;
			return false;
		}


		/**
		 * Represents a timeout associated with the blocking-
		 * operation on a semaphore.
		 */
		class Timeout : public Alarm
		{
			private:

				Timed_semaphore *_sem;       /* Semaphore we block on */
				Element         *_element;   /* Queue element timeout belongs to */
				bool             _triggered; /* Timeout expired */
				Time             _start;

			public:

				Timeout(Time duration, Timed_semaphore *s, Element *e);

				void discard();
				bool triggered() { return _triggered; }
				Time start()     { return _start;     }

			protected:

				bool on_alarm(unsigned) override
				{
					/* Abort blocking operation */
					_triggered = _sem->_abort(_element);
					return false;
				}
		};

	public:

		static void init(Env &);

		/**
		 * Constructor
		 *
		 * \param n  initial counter value of the semphore
		 */
		Timed_semaphore(int n = 0) : Semaphore(n) { }

		/**
		 * Decrements semaphore and blocks when it's already zero.
		 *
		 * \param t after t milliseconds of blocking a Timeout_exception is thrown.
		 *          if t is zero do not block, instead raise an
		 *          Nonblocking_exception.
		 * \return  milliseconds the caller was blocked
		 */
		Alarm::Time down(Alarm::Time t);


		/********************************
		 ** Base class implementations **
		 ********************************/

		void down() { Semaphore::down(); }
		void up()   { Semaphore::up();   }
};

#endif /* _INCLUDE__OS__TIMED_SEMAPHORE_H_ */
