#pragma once

namespace svr
{
	template <typename T>
	class ValueCache
	{
	private:
		mutable T m_value{};
		bool m_isEmpty;

	public:
		ValueCache() : m_value(), m_isEmpty(true)
		{}

		ValueCache(T& value) : m_value(value), m_isEmpty(false)
		{}

		ValueCache(T&& value) : m_value(std::move(value)), m_isEmpty(false)
		{}

		bool isEmpty() const { return m_isEmpty; }

		T& getValue() const
		{
			return m_value;
		}

		void setValue(const T& value)
		{
			flipEmpty();
			m_value = value;
		}

		void setValue(T&& value)
		{
			flipEmpty();
			m_value = std::move(value);
		}

		// USE CAREFULLY!
		// use only if a variable inside was changed externally via a pointer
		void forceUnsetEmpty() { m_isEmpty = false; }

		void operator=(const T& value) { setValue(value); }
		void operator=(T&& value) { setValue(std::move(value)); }

	private:
		void flipEmpty()
		{
			if (m_isEmpty)
				m_isEmpty = false;
		}
	};
}
