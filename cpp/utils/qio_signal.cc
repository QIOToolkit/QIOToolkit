
#include "utils/qio_signal.h"

#include <signal.h>

#include <iostream>

#include "utils/exception.h"
#include "utils/log.h"
#include "utils/timing.h"

namespace utils
{
Signal::Signal(Type type) : type_(type) { time_ = get_wall_time(); }

std::queue<Signal>& Signal::queue() { return queue_; }

Signal::Type Signal::get_type() { return type_; }

double Signal::get_time() { return time_; }

std::queue<Signal> Signal::queue_;

namespace
{
double signal_time = 0;
int signal_count = 0;

#ifdef SIGUSR1
void usr1_handler(int)
{
  LOG(INFO, "Received SIGUSR1, showing STATUS:");
  Signal::queue().push(Signal(Signal::STATUS));
}
#endif

#ifdef SIGUSR2
void usr2_handler(int)
{
  LOG(WARN, "Received SIGUSR2, halting...");
  Signal::queue().push(Signal(Signal::HALT));
}
#endif

void sigfpe_handler(int)
{
  try
  {
    throw ValueException(
        "Floating point operation exception happened, like overflow or "
        "divided by 0, please check your parameters");
  }
  catch (const ValueException& e)
  {
    std::cerr << kUserErrorTag << e.get_error_code_message() << kUserErrorTag
              << std::endl;
  }
  exit(SIGFPE);
}

void int_handler(int)
{
  double now = get_wall_time();
  if (now - signal_time < 2.0 /*seconds*/)
  {
    signal_count++;
  }
  else
  {
    signal_count = 1;
  }
  signal_time = now;

  if (signal_count == 1)
  {
    LOG(WARN, "Received first SIGINT, showing STATUS:");
    Signal::queue().push(Signal(Signal::STATUS));
  }
  else if (signal_count == 2)
  {
    LOG(WARN, "Received second SIGINT, halting...");
    Signal::queue().push(Signal(Signal::HALT));
  }
  else
  {
    LOG(FATAL, "Received third SIGINT, aborting immediately.");
    exit(EXIT_FAILURE);
  }
}

void term_handler(int)
{
  LOG(FATAL, "Received SIGTERM, aborting immediately.");
  exit(EXIT_FAILURE);
}

class SignalHandler
{
 public:
  SignalHandler()
  {
#ifdef SIGUSR1
    signal(SIGUSR1, usr1_handler);
#endif
#ifdef SIGUSR2
    signal(SIGUSR2, usr2_handler);
#endif
    signal(SIGINT, int_handler);
    signal(SIGTERM, term_handler);
    signal(SIGFPE, sigfpe_handler);
  }
};

SignalHandler signal_handler;

}  // namespace
}  // namespace utils
