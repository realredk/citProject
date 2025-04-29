/*
 * Copyright ©2025 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 5950 for use solely during Spring Semester 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <unistd.h>
#include <iostream>

#include "./ThreadPool.hpp"

namespace searchserver {

// This is the thread start routine, i.e., the function that threads
// are born into.
void* thread_loop(void* t_pool);

ThreadPool::ThreadPool(size_t num_threads)
    : q_lock_(),
      q_cond_(),
      work_queue_(),
      killthreads_(false),
      num_threads_(num_threads),
      thread_vec_(num_threads) {
  // Initialize our member variables.

  // TODO
  pthread_mutex_init(&q_lock_, nullptr);
  pthread_cond_init(&q_cond_, nullptr);

  // Spawn worker threads
  for (size_t i = 0; i < num_threads_; ++i) {
    if (pthread_create(&thread_vec_[i], nullptr, thread_loop, this) != 0) {
      throw std::runtime_error("Failed to create thread");
    }
  }
}

ThreadPool::~ThreadPool() {
  // TODO
  // Signal threads to shutdown
  pthread_mutex_lock(&q_lock_);
  killthreads_ = true;
  pthread_cond_broadcast(&q_cond_);
  pthread_mutex_unlock(&q_lock_);

  // Join all threads
  for (pthread_t& tid : thread_vec_) {
    pthread_join(tid, nullptr);
  }

  // Clean up
  pthread_mutex_destroy(&q_lock_);
  pthread_cond_destroy(&q_cond_);
}

// Enqueue a Task for dispatch.
void ThreadPool::dispatch(Task t) {
  // TODO
  // Add task to queue and notify a worker
  pthread_mutex_lock(&q_lock_);
  work_queue_.push_back(t);
  pthread_cond_signal(&q_cond_);
  pthread_mutex_unlock(&q_lock_);
}

// This is the main loop that all worker threads are born into.  They
// wait for a signal on the work queue condition variable, then they
// grab work off the queue.  Threads return (i.e., kill themselves)
// when they notice that killthreads_ is true.
void* thread_loop(void* t_pool) {
  // TODO
  // Recover the ThreadPool pointer
  ThreadPool* pool = static_cast<ThreadPool*>(t_pool);

  while (true) {
    // 1. Lock the queue before touching shared data
    pthread_mutex_lock(&pool->q_lock_);

    // 2. Wait until there's either a task or a shutdown
    while (pool->work_queue_.empty() && !pool->killthreads_) {
      pthread_cond_wait(&pool->q_cond_, &pool->q_lock_);
    }

    // 3. If we're shutting down and no tasks remain, clean up and exit
    if (pool->killthreads_ && pool->work_queue_.empty()) {
      pthread_mutex_unlock(&pool->q_lock_);
      break;
    }

    // 4. Otherwise grab the next task
    auto task = pool->work_queue_.front();
    pool->work_queue_.pop_front();

    // 5. Unlock so others can add or take tasks
    pthread_mutex_unlock(&pool->q_lock_);

    // 6. Run the task
    task.func_(task.arg_);
  }
  // 7. Clean up and exit
  return nullptr;
}

}  // namespace searchserver
