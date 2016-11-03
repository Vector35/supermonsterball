#include <mutex>
#include "processingthread.h"

using namespace std;


ProcessingThread* ProcessingThread::m_instance = nullptr;


ProcessingThread::ProcessingThread(): m_running(true)
{
	m_instance = this;
	m_thread = thread([this]() { ThreadProc(); });
}


ProcessingThread::~ProcessingThread()
{
	m_thread.join();
	m_instance = nullptr;
}


void ProcessingThread::Process(const function<void()>& action)
{
	unique_lock<mutex> lock(m_mutex);
	condition_variable finish;

	bool done = false;
	bool except = false;
	ProcessingException pe;
	m_queue.push([&]() {
		try
		{
			action();
		}
		catch (exception& e)
		{
			pe = ProcessingException(e.what());
			except = true;
		}

		unique_lock<mutex> innerLock(m_mutex);
		done = true;
		finish.notify_one();
	});

	m_queueEvent.notify_one();

	finish.wait(lock, [&]() { return done; });

	if (except)
		throw pe;
}


void ProcessingThread::ThreadProc()
{
	while (m_running)
	{
		unique_lock<mutex> lock(m_mutex);
		m_queueEvent.wait(lock, [this]() { return (!m_running) || (m_queue.size() > 0); });
		if (!m_running)
			break;

		function<void()> entry = m_queue.front();
		m_queue.pop();

		lock.unlock();

		entry();
	}
}


void ProcessingThread::Stop()
{
	unique_lock<mutex> lock(m_mutex);
	m_running = false;
	m_queueEvent.notify_all();
}
