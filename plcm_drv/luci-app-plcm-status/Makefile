include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-plcm-status
PKG_RELEASE:=1
PKG_LICENSE:=Apache-2.0

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-plcm-status
	SECTION:=luci
	CATEGORY:=LuCI
	SUBMENU:=3. Applications
	TITLE:=LuCI support for the PLCM status display
	DEPENDS:=+plcm-status
	PKGARCH:=all
endef

define Build/Compile
endef

define Package/luci-app-plcm-status/install
	$(INSTALL_DIR) $(1)/www/luci-static/resources/view
	$(INSTALL_DATA) ./htdocs/luci-static/resources/view/plcm-status.js \
		$(1)/www/luci-static/resources/view/plcm-status.js
	$(INSTALL_DIR) $(1)/usr/share/luci/menu.d
	$(INSTALL_DATA) ./root/usr/share/luci/menu.d/luci-app-plcm-status.json \
		$(1)/usr/share/luci/menu.d/luci-app-plcm-status.json
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./root/usr/share/rpcd/acl.d/luci-app-plcm-status.json \
		$(1)/usr/share/rpcd/acl.d/luci-app-plcm-status.json
endef

$(eval $(call BuildPackage,luci-app-plcm-status))
