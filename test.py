import wayfire_socket as ws
import os

msg = ws.get_msg_template('ueberzugpp/set_offset')
msg["data"]["app-id"] = "ueberzugpp_123"
msg["data"]["x"] = 10
msg["data"]["y"] = 50

socket = ws.WayfireSocket(os.getenv('WAYFIRE_SOCKET'))
print(socket.send_json(msg))
