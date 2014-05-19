#include <node.h>
#include <v8.h>
#include <queue>
#include <string>

#include "daemon.h"

using namespace v8;

uv_loop_t *loop;


/*
 *  setLogCallback
 *  Callback hook for pilights log function 
 */
Persistent<Function> jsLogCallback;
uv_async_t async_log;
struct log_entry {
  int prio;
  std::string line;
} typedef log_entry;

// logentries buffer and backbuffer
pthread_mutex_t logentries_lock;
int logentries_write_buffer = 0;
std::queue <log_entry> logentries[2];

// this function is called by pilight if a message gets logged.
void logcallback(int prio, char* line) {
  // query the message to be send to v8
  log_entry entry;
  entry.prio = prio;
  entry.line = line;
  pthread_mutex_lock(&logentries_lock);
  // write the message into the backbuffer
  logentries[logentries_write_buffer].push(entry);
  pthread_mutex_unlock(&logentries_lock);
  // and notify the main loop that the backbuffer has entries
  uv_async_send(&async_log);
}

// this function is called by async in the main event loop
void call_logcallback(uv_async_t *handle, int status /*UNUSED*/) {
  // switch bufferes, so we don't block other threads, while calling the v8 callbacks
  pthread_mutex_lock(&logentries_lock);
  int read_buffer = logentries_write_buffer;
  logentries_write_buffer = (logentries_write_buffer+1) % 2;
  pthread_mutex_unlock(&logentries_lock);
  // call the v8 callback with all entries the old buffer:
  while(logentries[read_buffer].size() > 0) {
    log_entry entry = logentries[read_buffer].front();
    logentries[read_buffer].pop();
    const unsigned argc = 2;
    Local<Value> argv[argc] = { 
     Local<Value>::New(Number::New(entry.prio)),
     Local<Value>::New(String::New(entry.line.c_str())),
    };
    jsLogCallback->Call(Context::GetCurrent()->Global(), argc, argv);
  }
}

// v8 function to register the callback
Handle<Value> setLogCallback(const Arguments& args) {
  HandleScope scope;
  Local<Function> cb = Local<Function>::Cast(args[0]);
  jsLogCallback = Persistent<Function>::New(cb);
  log_setcallback(logcallback);
  return scope.Close(Undefined());
}

/*
 *  Callback hook for pilights broadcast function 
 */
Persistent<Function> jsBroadcastCallback;
uv_async_t async_broadcast;

pthread_mutex_t broadcasts_lock;
int broadcasts_write_buffer = 0;
std::queue <std::string> broadcasts[2];

// Called by pilight on broadcast -> triggers async node.js callback
void broadcastCallback(char* message) {
  pthread_mutex_lock(&broadcasts_lock);
  broadcasts[broadcasts_write_buffer].push(std::string(message));
  pthread_mutex_unlock(&broadcasts_lock);  
  uv_async_send(&async_broadcast);
}

//this function is valled by async in the main event loop
void call_broadcastCallback(uv_async_t *handle, int status /*UNUSED*/) {
    //switch bufferes, so we don't block the logging thread for a longger time
  //when calling the v8 callbacks
  pthread_mutex_lock(&broadcasts_lock);
  int read_buffer = broadcasts_write_buffer;
  broadcasts_write_buffer = (broadcasts_write_buffer+1) % 2;
  pthread_mutex_unlock(&broadcasts_lock);

  while(broadcasts[read_buffer].size() > 0) {
    std::string message = broadcasts[read_buffer].front();
    broadcasts[read_buffer].pop();
    const unsigned argc = 1;
    Local<Value> argv[argc] = { 
      Local<Value>::New(String::New(message.c_str()))
    };
    jsBroadcastCallback->Call(Context::GetCurrent()->Global(), argc, argv);
  }
}

// reigisters the node.js callback for broadcasts
Handle<Value> setBroadcastCallback(const Arguments& args) {
  HandleScope scope;
  Local<Function> cb = Local<Function>::Cast(args[0]);
  jsBroadcastCallback = Persistent<Function>::New(cb);
  broadcast_setcallback(broadcastCallback);
  return scope.Close(Undefined());
}

/*
* Start daemon
*/
uv_work_t start_daemon_req;
std::string settingsContent;

void start_daemon_worker(uv_work_t *req) {
  start_daemon(settingsContent.c_str());
}

void start_daemon_worker_after(uv_work_t *req, int status) {
  // send all remaining log entries to node
  call_logcallback(NULL, 0);
  // send all remaining broadcasts to node
  call_broadcastCallback(NULL, 0);
  // destroy the mutextes
  pthread_mutex_destroy(&logentries_lock);
  pthread_mutex_destroy(&broadcasts_lock);
  // cleanup the worker
  uv_close((uv_handle_t*) &async_log, NULL);
  uv_close((uv_handle_t*) &async_broadcast, NULL);
};

Handle<Value> startDaemon(const Arguments& args) {
  HandleScope scope;
  String::Utf8Value sc(args[0]->ToString());
  settingsContent = *sc;
  uv_queue_work(loop, &start_daemon_req, start_daemon_worker, start_daemon_worker_after);
  return scope.Close(Undefined());
}


Handle<Value> sendMessage(const Arguments& args) {
  HandleScope scope;
  String::Utf8Value message(args[0]->ToString());
  send_message(*message);	
  return scope.Close(Undefined());
}

void init(Handle<Object> exports) {
  pthread_mutex_init(&logentries_lock, NULL);
  pthread_mutex_init(&broadcasts_lock, NULL);

  loop = uv_default_loop();
  uv_async_init(loop, &async_log, call_logcallback);
  uv_async_init(loop, &async_broadcast, call_broadcastCallback);

  exports->Set(String::NewSymbol("startDaemon"),
      FunctionTemplate::New(startDaemon)->GetFunction());
  exports->Set(String::NewSymbol("setLogCallback"),
      FunctionTemplate::New(setLogCallback)->GetFunction());
  exports->Set(String::NewSymbol("setBroadcastCallback"),
      FunctionTemplate::New(setBroadcastCallback)->GetFunction());
   exports->Set(String::NewSymbol("sendMessage"),
      FunctionTemplate::New(sendMessage)->GetFunction());
}


NODE_MODULE(pilightjs, init)