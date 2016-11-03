#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <string>


class ProcessingException: public std::exception
{
	std::string m_message;

public:
	ProcessingException(): m_message("exception") {}
	ProcessingException(const std::string& msg): m_message(msg) {}
	virtual const char* what() const noexcept override { return m_message.c_str(); }
};


class ProcessingThread
{
	std::queue<std::function<void()>> m_queue;
	std::mutex m_mutex;
	std::condition_variable m_queueEvent;
	std::thread m_thread;
	bool m_running;

	static ProcessingThread* m_instance;

	void ThreadProc();

public:
	ProcessingThread();
	~ProcessingThread();

	void Process(const std::function<void()>& action);

	void Stop();

	static ProcessingThread* Instance() { return m_instance; }
};
