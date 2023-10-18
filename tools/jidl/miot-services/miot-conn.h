
#ifndef MIOT_CONN_H
#define MIOT_CONN_H

#include "msgproto/miotmsg.pb-c.h"

class MiotConnect {
 public:
  static MiotConnect* Create(FeatureContext ctx);
  static MiotConnect* From(FeatureInstanceHandle handle);

  void send(const Miotmsg* msg, FtCallbackId success, FtCallbackId fail, FtCallbackId complete, ft_value_t translate(FeatureInstanceHandle handle, ft_value_t value));

  void listen(const Miotmsg* pmsg, const char* pname, FtCallbackId cb);

 private:
  // ....
};


#endif
