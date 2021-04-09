/**
 * File:        pms.cpp
 * Project:     PRL-2021-MPI-PMS
 * Author:      Jozef Méry - xmeryj00@vutbr.cz
 * Date:        2.4.2021
 */

// std lib
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <chrono>

// OpenMPI
#include <mpi.h>

// local
#include "pms.hpp"

using namespace PMS;

Processors::Base::Base(const Pid pid, const int count) 
  
  : pid_{ pid }
  , count_{ count }
  , pipe_select_{ PipeSelect::Upper }
  , sent_to_selected_{ 0 }

{
  // pid sanity check
  if(pid < FIRST_PID || pid > LAST_PID) {

    Ss msg;
    msg << "Expected pid from range: <0, " << LAST_PID << ">, got: " << pid;

    abort(msg.str(), ExitCode::PROCESS_ERR);
  }
}

/* static */ void Processors::Base::print_sequence(const Sequence& sequence, 
  const char* const delimiter) {

  if(sequence.empty()) { return; }

  // print all items but last with delimiters
  std::copy(sequence.begin(), sequence.end() - 1, std::ostream_iterator<int>{ std::cout, delimiter });
  // print last item without a delimiter after it, and add a newline
  // cast char to int to treat it as a number, not a letter
  std::cout << static_cast<int>(*(sequence.end() - 1)) << "\n";
}

String Processors::Base::format_error(const String& msg) const {

  Ss ss;

  ss << pid_ << ": [ERROR]: " << msg << "\n";

  return ss.str();
}

void Processors::Base::abort(const String& msg, const ExitCode exitCode) const {

  std::cerr << format_error(msg);
  MPI_Abort(MPI_COMM_WORLD, static_cast<int>(exitCode));
}

Pid Processors::Base::next_pid() const {
  
  return pid_ + 1;
}

Pid Processors::Base::previous_pid() const {

  return pid_ - 1;
}

void Processors::Base::send_to_next_p(const Message& msg) {

  MPI_Send(&(msg.primitive), 1, MPI_UNSIGNED_CHAR, next_pid(), 
    static_cast<int>(msg.pipe), MPI_COMM_WORLD);
  
  ++sent_to_selected_;
}

Message Processors::Base::receive_from_prev_p() {

  MPI_Status status;
  Primitive primitive;

  MPI_Recv(&primitive, 1, MPI_UNSIGNED_CHAR, previous_pid(), MPI_ANY_TAG, MPI_COMM_WORLD, &status);

  return { primitive, static_cast<PipeSelect>(status.MPI_TAG) };
}

void Processors::Base::toggle_pipe() {

  if(pipe_select_ == PipeSelect::Upper) {

    pipe_select_ = PipeSelect::Lower;
  
  } else {

    pipe_select_ = PipeSelect::Upper;
  }

  sent_to_selected_ = 0;

  clear_taken();
}

int Processors::Base::in_seq_len() const {

  return pow(2, pid_ - 1);
}

int Processors::Base::out_seq_len() const {

  return pow(2, pid_ );
}

bool Processors::Base::should_toggle_pipe() const {

  return sent_to_selected_ >= out_seq_len();
}

Processors::First::First(const Pid pid, const int count) 

  : Base{ pid, count }

{}

void Processors::First::run() {

  check_processes();
  read_input();
  check_input();
  if constexpr (!BENCH) {

    print_sequence(input_, " ");
  }
  sort();
}

void Processors::First::check_processes() const {

  if(count_ == EXPECTED_PROCESSES) { return; }

  Ss msg;
  msg << "Expected " << EXPECTED_PROCESSES << " processes, got: " << count_;

  abort(msg.str(), ExitCode::PROCESS_ERR);
}

void Processors::First::read_input() {

  std::ifstream file { INPUT_FILE, std::ios::binary };

  if(!file.good()) {

    const auto msg{ "The numbers input file was not found in the application directory" };

    abort(msg, ExitCode::INPUT_ERR);
  }

  input_ = { std::istreambuf_iterator<char>(file), {} };
}

void Processors::First::check_input() const {

  if(input_.size() == EXPECTED_NUMS) { return; }
 
  Ss msg;
  msg << "Expected " << EXPECTED_NUMS << " input numbers, got: " << input_.size(); 
  
  abort(msg.str(), ExitCode::INPUT_ERR);
}

void Processors::First::sort() {

  for(const auto& num : input_) {

    send_to_next_p({ num, pipe_select_ });

    if(should_toggle_pipe()) {

      toggle_pipe();
    }
  }
}

Processors::Mid::Mid(const Pid pid, const int count) 
  
  : Base{ pid, count }
  , started_{ false }
{}

void Processors::Mid::run() {

  sort();
}

void Processors::Mid::sort() {

  for(int i{ 0 }; i < EXPECTED_NUMS + in_seq_len(); ++i) {

    if(i < EXPECTED_NUMS) {

      save_message(receive_from_prev_p());
    }

    if(can_sort_begin()) {

      start_sort();
    }

    if(!started_) { continue; }

    send_to_next_p({ next_primitive(), pipe_select_ });

    if(should_toggle_pipe()) {

      toggle_pipe();
    }
  }
}

bool Processors::Mid::can_sort_begin() const {

  const auto len1{ pipes_[0].queue.size() };
  const auto len2{ pipes_[1].queue.size() };

  return len1 >= in_seq_len() && len2 >= 1;
}

void Processors::Mid::start_sort() {

  started_ = true;
}

Primitive Processors::Mid::next_primitive() {
  
  // shorthands
  auto& p1{ pipes_[0] };
  auto& p2{ pipes_[1] };
  
  // handle edge-cases
  for(int i{ 0 }; i < 2; ++i) {

    if(pipes_[i].taken >= in_seq_len() || pipes_[i].queue.empty()) {

      // other pipe must have a value
      const auto otherId{ (i + 1) % 2 };
      auto& pipe{ pipes_[otherId] };

      pipe.taken++;

      const auto val{ pipe.queue.front() };
      pipe.queue.pop();

      return val;
    }
  }

  // assume 1st pipe selection
  int pipeId{ 0 };

  // pre-select values
  auto val1{ p1.queue.front() };
  auto val2{ p2.queue.front() };

  if(val1 > val2) {

    pipeId = 1;
  }

  auto& pipe{ pipes_[pipeId] };

  pipe.taken++;
  const auto val{ pipe.queue.front() };
  pipe.queue.pop();

  return val;
}

void Processors::Mid::save_message(const Message& msg) {

  auto& pipe{ pipes_[static_cast<int>(msg.pipe)] };

  pipe.queue.push(msg.primitive);
}

void Processors::Mid::clear_taken() {

  // if(pid_ == LAST_PID) { std::cout << "foo\n"; }

  pipes_[0].taken = 0;
  pipes_[1].taken = 0;
}

Processors::Last::Last(const Pid pid, const int count) 
  
  : Mid{ pid, count }

{}

void Processors::Last::run() {

  if constexpr (!BENCH) {

    sort();
    print_sequence(output_, "\n");
  
  } else {

    // Benchmarking code adopted from:
    // https://stackoverflow.com/a/22387757/5150211
    using std::chrono::high_resolution_clock;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    const auto t1 = high_resolution_clock::now();
    sort();
    const auto t2 = high_resolution_clock::now();

    // getting number of milliseconds as a double
    const duration<double, std::milli> diff = t2 - t1;

    std::cout << diff.count() << "ms\n";
  }
}

void Processors::Last::sort() {

  for(int i{ 0 }; i < EXPECTED_NUMS + in_seq_len(); ++i) {

    if(i < EXPECTED_NUMS) {

      save_message(receive_from_prev_p());
    }

    if(can_sort_begin()) {

      start_sort();
    }

    if(!started_) { continue; }

    output_.push_back(next_primitive());
  }
}

Primitive Processors::Last::next_primitive() {
  
  // shorthands
  auto& p1{ pipes_[0] };
  auto& p2{ pipes_[1] };
  
  // handle edge-cases
  for(int i{ 0 }; i < 2; ++i) {

    if(pipes_[i].queue.empty()) {

      // other pipe must have a value
      const auto otherId{ (i + 1) % 2 };
      auto& pipe{ pipes_[otherId] };

      const auto val{ pipe.queue.front() };
      pipe.queue.pop();

      return val;
    }
  }

  // assume 1st pipe selection
  int pipeId{ 0 };

  // pre-select values
  auto val1{ p1.queue.front() };
  auto val2{ p2.queue.front() };

  if(val1 > val2) {

    pipeId = 1;
  }

  auto& pipe{ pipes_[pipeId] };

  const auto val{ pipe.queue.front() };
  pipe.queue.pop();

  return val;
}

/* static */ Processor App::get_processor(const Pid pid, const int count) {

  using namespace Processors;

  if(pid == FIRST_PID) {

    return std::make_unique<First>(pid, count);
  
  } else if(pid == LAST_PID) {

    return std::make_unique<Last>(pid, count);
  }

  return std::make_unique<Mid>(pid, count); 
}

App::App(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  // initialize to invalid values
  int count{ 0 };
  Pid pid{ -1 };
 
  // fetch process count and current process id
  MPI_Comm_size(MPI_COMM_WORLD, &count);
  MPI_Comm_rank(MPI_COMM_WORLD, &pid);

  processor_ = get_processor(pid, count);
}

App::~App() noexcept {

  MPI_Finalize();
}

void App::run() {

  processor_->run();
}

int main(int argc, char* argv[]) {

  App{ argc, argv }.run();
  return static_cast<int>(ExitCode::OK);
}