IF(INSTALL_EXTENSIONLESS_HEADERS)
  INSTALL_FILES(/include/Wt/Payment "^[^.]+([.][^C~])?$")
ELSE(INSTALL_EXTENSIONLESS_HEADERS)
  INSTALL_FILES(/include/Wt/Payment "^[^.]+[.]h$")
ENDIF(INSTALL_EXTENSIONLESS_HEADERS)
