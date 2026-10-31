include $(TOPDIR)/rules.mk

PKG_NAME:=nomadix-platform
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Nomadix firmware project

include $(INCLUDE_DIR)/package.mk

define Package/nomadix-platform
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Nomadix FW-7582 platform services
  DEPENDS:=@TARGET_x86 +kmod-hwmon-coretemp +kmod-hwmon-w83627ehf
endef

define Package/nomadix-platform/description
  Fail-safe dynamic fan control and platform diagnostics for the
  Lanner/Nomadix FW-7582 running the FOXNODIX firmware.
endef

define Package/nomadix-platform-maintenance
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Nomadix FW-7582 maintenance tools
  DEPENDS:=@TARGET_x86 +nomadix-platform +flashrom
endef

define Package/nomadix-platform-maintenance/description
  Optional firmware read-back and verification tools. This package is
  deliberately separate from the normal router runtime.
endef

define Package/nomadix-platform/conffiles
/etc/config/nomadix-platform
endef

define Package/nomadix-platform/prerm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || /etc/init.d/nomadix-platform stop
exit 0
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./LICENSE $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) \
		-Wall -Wextra -Werror -std=c99 \
		-o $(PKG_BUILD_DIR)/nomadix-fand \
		$(PKG_BUILD_DIR)/nomadix-fand.c $(TARGET_LDFLAGS)
endef

define Package/nomadix-platform/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/nomadix-platform.config $(1)/etc/config/nomadix-platform
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/nomadix-platform.init $(1)/etc/init.d/nomadix-platform
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/nomadix-fand $(1)/usr/sbin/nomadix-fand
	$(INSTALL_BIN) ./files/nomadix-platform-status $(1)/usr/sbin/nomadix-platform-status
endef

define Package/nomadix-platform-maintenance/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/nomadix-flash-readback $(1)/usr/sbin/nomadix-flash-readback
endef

$(eval $(call BuildPackage,nomadix-platform))
$(eval $(call BuildPackage,nomadix-platform-maintenance))
