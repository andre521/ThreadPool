#ifndef CTHREADPOOLITEM
#define CTHREADPOOLITEM
#include<atomic>	//atomic<bool>
#include<functional>	//function
#include<memory>	//unique_ptr
#include<utility>	//move
#include"../../lib/header/thread/CSemaphore.h"
#include"../../lib/header/thread/CSmartThread.h"
#include"CThreadPoolCommun.h"

namespace nThread
{
	template<class T>
	struct IThreadPoolItemExecutorBase;

	class CThreadPoolItem
	{
		std::unique_ptr<CThreadPoolCommunBase> commun_;	//communicate with CThreadPool
		bool destructor_;
		std::unique_ptr<IThreadPoolItemExecutorBase<void>> exec_;
		CSemaphore wait_;
		CSmartThread thr_;	//first destroying, no other data member could put under this one
		void loop_();
		void waiting_();
		void wake_();
	public:
		CThreadPoolItem();
		CThreadPoolItem(const CThreadPoolItem &)=delete;
		template<class Func,class ... Args>
		void assign(Func &&,Args &&...);
		template<class Func,class ... Args>
		void assign_and_detach(Func &&,Args &&...);
		void join();	//after calling this, CThreadPoolItem will be pushed into waitingQue_
						//it also means assign will be called in the subsequent (if has)
		bool joinable() const noexcept;
		void setCommun(std::unique_ptr<CThreadPoolCommunBase> &&);
		CThreadPoolItem& operator=(const CThreadPoolItem &)=delete;
		~CThreadPoolItem();
	};

	template<class Ret>
	struct IThreadPoolItemExecutorBase	//I give up to use Non-Virtual Interface Idiom here
										//because this is a abstract base struct
	{
		virtual void exec()=0;
		virtual bool is_running() const noexcept=0;
		bool joinable() const noexcept
		{
			return joinable_();
		}
		virtual void wait()=0;
		virtual ~IThreadPoolItemExecutorBase()=0;
	protected:
		virtual bool joinable_() const noexcept
		{
			return false;
		}
	};

	class CThreadPoolItemExecutorDetach:public IThreadPoolItemExecutorBase<void>
	{
		CThreadPoolCommunBase *commun_;
		CSemaphore complete_;
		std::function<void()> func_;
	public:
		template<class Func,class ... Args>
		CThreadPoolItemExecutorDetach(CThreadPoolCommunBase *,Func &&,Args &&...);
		CThreadPoolItemExecutorDetach(const CThreadPoolItemExecutorDetach &)=delete;
		void exec() override;
		bool is_running() const noexcept override	//only the destructor of CThreadPoolItem will call this
		{
			return !complete_.count();
		}
		void wait() override	//only the destructor of CThreadPoolItem will call this
		{
			complete_.wait();
		}
		CThreadPoolItemExecutorDetach& operator=(const CThreadPoolItemExecutorDetach &)=delete;
	};

	class CThreadPoolItemExecutorJoin:public IThreadPoolItemExecutorBase<void>
	{
		CThreadPoolCommunBase *commun_;
		CSemaphore complete_;
		std::function<void()> func_;
		std::atomic<bool> running_;
	protected:
		bool joinable_() const noexcept override
		{
			return true;
		}
	public:
		template<class Func,class ... Args>
		CThreadPoolItemExecutorJoin(CThreadPoolCommunBase *,Func &&,Args &&...);
		CThreadPoolItemExecutorJoin(const CThreadPoolItemExecutorJoin &)=delete;
		void exec() override;
		bool is_running() const noexcept override
		{
			return running_;
		}
		void wait() override;
		CThreadPoolItemExecutorJoin& operator=(const CThreadPoolItemExecutorJoin &)=delete;
	};
}

#include"CThreadPoolItem.cpp"

#endif