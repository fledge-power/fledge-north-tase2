#include "tase2.hpp"

TASE2OutstandingCommand::TASE2OutstandingCommand (const std::string& domain,
                                                  const std::string& name,
                                                  int cmdExecTimeout,
                                                  bool isSelect)
    : m_domain (domain), m_name (name), m_cmdExecTimeout (cmdExecTimeout),
      m_state (1)
{
    m_commandRcvdTime = Hal_getTimeInMs ();
    m_nextTimeout = m_commandRcvdTime + (m_cmdExecTimeout * 1000);
}

bool
TASE2OutstandingCommand::hasTimedOut (uint64_t currentTime)
{
    return (currentTime > m_nextTimeout);
}
