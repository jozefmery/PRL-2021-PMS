/**
 * File:        pms.hpp
 * Project:     PRL-2021-MPI-PMS
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        2.4.2021
 */

#pragma once

// std lib
#include <vector>
#include <string>
#include <tuple>
#include <sstream>
#include <queue>
#include <array>
#include <memory>

namespace PMS {

constexpr auto BENCH = false;

enum class ExitCode : int {

  OK          = 0,
  INPUT_ERR   = 1,
  PROCESS_ERR = 2
};

// assignment-based constants
constexpr auto INPUT_FILE = "./numbers";
constexpr auto EXPECTED_NUMS = 16;
constexpr auto EXPECTED_PROCESSES = 5;  // log2(n) + 1 = log2(16) + 1 = 4 + 1 = 5

// give raw types meaning
using Primitive = unsigned char;
using Sequence  = std::vector<Primitive>;
using Input     = Sequence;
using Output    = Sequence;
using PipeQ     = std::queue<Primitive>;
using Pid       = int;
using String    = std::string;

struct Pipe {

  PipeQ queue;
  int taken = 0;
};

using Pipes = std::array<Pipe, 2>;

// helper constants
constexpr Pid FIRST_PID = 0; 
constexpr Pid LAST_PID  = EXPECTED_PROCESSES - 1; 

// shorthand
// use string streams for improved string composition
using Ss = std::stringstream;

enum class PipeSelect : int {

  Upper = 0,
  Lower = 1
};

struct Message {

  Primitive primitive;
  PipeSelect pipe;
};

namespace Processors {

/* interface */ class Base {

public /* ctors, dtor */:

  explicit Base(const Pid pid, const int count);

  // every interface needs a virtual destructor
  virtual ~Base() = default;

public /* methods */:

  virtual void run() = 0;

protected /* static functions */:

  static void print_sequence(const Sequence& sequence, const char* const delimiter);

protected /* methods */:

  String format_error(const String& msg) const;
  void abort(const String& message, const ExitCode exitCode) const;
  Pid next_pid() const;
  Pid previous_pid() const;
  void send_to_next_p(const Message& msg);
  Message receive_from_prev_p();
  void toggle_pipe();
  virtual void clear_taken() = 0;
  int in_seq_len() const;
  int out_seq_len() const;
  bool should_toggle_pipe() const;

protected /* members */:

  Pid pid_;
  int count_;
  PipeSelect pipe_select_;
  int sent_to_selected_;
};

class First : public Base {

public /* ctors, dtor */:

  explicit First(const Pid pid, const int count);

public /* methods */:

  virtual void clear_taken() override {};
  virtual void run() override;

private /* methods */:

  void check_processes() const;
  void read_input();
  void check_input() const;
  void sort();

private /* members */:

  Input input_;
};

class Mid : public Base {

public /* ctors, dtor */:

  explicit Mid(const Pid pid, const int count);

public /* methods */:

  virtual void run() override;

private /* methods */:

  void sort();

protected /* methods */:

  bool can_sort_begin() const;
  void start_sort();
  Primitive next_primitive();
  void save_message(const Message& msg);
  virtual void clear_taken() override;

protected /* members */:

  Pipes pipes_;
  bool started_;
};

class Last : public Mid {

public /* ctors, dtor */:

  explicit Last(const Pid pid, const int count);

private /* methods */:

  virtual void run() override;
  void sort();
  Primitive next_primitive();

private /* members */:

  Output output_;
};
}

using Processor = std::unique_ptr<Processors::Base>;

class App {

public /* ctors, dtor */:

  explicit App(int argc, char* argv[]);

  ~App() noexcept;

public /* methods */:

  void run();

private /* static functions */:

  static Processor get_processor(const Pid pid, const int count);

private /* members */:

  Processor processor_;
};

}