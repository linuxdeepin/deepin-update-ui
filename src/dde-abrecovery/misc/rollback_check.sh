#!/bin/bash

can_restore=$(qdbus --literal --system org.deepin.dde.Lastore1 /org/deepin/dde/Lastore1 org.deepin.dde.Lastore1.Manager.CanRollback)
if [ "$can_restore" = "false" ];then
  echo "can not restore"
  exit 1
fi
echo "rollback can process"
exit 0