#pragma once

#include "asio/steady_timer.hpp"

namespace server
{
	namespace core
	{
		// public
		template<typename lambda>
		auto async(lambda task);

		template<typename duration = std::chrono::milliseconds, typename lambda>
		auto async_after(duration delay, lambda task) -> std::future<void>;

		template<typename duration = std::chrono::milliseconds, typename lambda>
		auto async_every(duration delay, lambda task) -> std::future<void>;

		template<typename duration = std::chrono::milliseconds, typename lambda>
		auto async_every(duration delay, volatile int& stop_signal, lambda task) -> std::future<void>;


		// detail
		template<typename duration>
		struct async_timer_frame
		{
			asio::steady_timer timer;
			duration delay;
			asio::error_code error_code;

			async_timer_frame(duration delay) : timer(io_context), delay(delay) {}

			bool await_ready()
			{
				return delay.count() == 0;
			}

			void await_suspend(std::experimental::coroutine_handle<> coro)
			{
				timer.expires_from_now(delay);
				timer.async_wait([this, coro](auto error_code) { this->error_code = error_code; coro.resume(); });
			}

			void await_resume()
			{
				if (error_code)
				{
					throw asio::system_error(error_code);
				}
			}
		};

		template<typename duration>
		auto async_timer(duration delay)
		{
			return async_timer_frame<duration>(delay);
		}

		template<typename lambda>
		auto async(lambda task)
		{
			asio::co_spawn(io_context, [&task]() -> asio::awaitable<void>
			{
				auto executor = co_await asio::this_coro::executor;
				task();
			}, asio::detached);
		}

		template<typename duration, typename lambda>
		auto async_after(duration delay, lambda task) -> std::future<void>
		{
			co_await async_timer(delay);
			task();
		}

		template<typename duration, typename lambda>
		auto async_every(duration delay, lambda task) -> std::future<void>
		{
			while (true)
			{
				co_await async_timer(delay);
				task();
			}
		}

		template<typename duration, typename lambda>
		auto async_every(duration delay, volatile int& stop_signal, lambda task) -> std::future<void>
		{
			while (!stop_signal)
			{
				co_await async_timer(delay);
				task();
			}
		}
	}
}