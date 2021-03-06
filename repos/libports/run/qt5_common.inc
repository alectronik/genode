source ${genode_dir}/repos/libports/run/qt5_drivers.inc

set feature(Input) 1
set feature(Framebuffer) 1
set feature(Timer) 1

#
# Build
#

proc qt5_build_components { feature_arg } {

	upvar $feature_arg feature

	set build_components {
		core
		init
	}

	append build_components [drivers_build_components feature]

	append build_components {
		server/nitpicker
		server/report_rom
		app/floating_window_layouter
		app/decorator
		app/pointer
		server/wm
	}

	return $build_components
}


#
# Configuration
#

proc qt5_parent_provides { feature_arg } {

	upvar $feature_arg feature

	set parent_provides [drivers_parent_provides feature]

	return $parent_provides
}

proc qt5_start_nodes { feature_arg } {

	upvar $feature_arg feature

	set start_nodes [drivers_start_nodes feature]

	append start_nodes {
		<start name="nitpicker">
			<resource name="RAM" quantum="1M"/>
			<provides><service name="Nitpicker"/></provides>
			<config>
				<domain name="pointer"   layer="1" xray="no" origin="pointer" />
				<domain name="decorator" layer="2" xray="opaque" />
				<domain name="default"   layer="2" />

				<policy label="pointer"         domain="pointer"/>
				<policy label=""                domain="default"/>
				<policy label="wm -> decorator" domain="decorator"/>

				<global-key name="KEY_SCROLLLOCK" operation="xray" />
				<global-key name="KEY_SYSRQ"      operation="kill" />
				<global-key name="KEY_PRINT"      operation="kill" />
				<global-key name="KEY_F11"        operation="kill" />
				<global-key name="KEY_F12"        operation="xray" />
			</config>
		</start>

		<start name="pointer">
			<resource name="RAM" quantum="1M"/>
			<route>
				<service name="Nitpicker"> <child name="nitpicker"/> </service>
				<any-service> <parent/> <any-child/> </any-service>
			</route>
		</start>

		<start name="wm">
			<resource name="RAM" quantum="16M"/>
			<provides><service name="Nitpicker"/></provides>
			<route>
				<service name="Nitpicker"> <child name="nitpicker"/> </service>
				<any-service> <parent/> <any-child/> </any-service>
			</route>
		</start>
	}

	return $start_nodes
}

#
# Boot modules
#

proc qt5_boot_modules { feature_arg } {

	upvar $feature_arg feature

	set boot_modules {
		core
		init
	}

	append boot_modules [drivers_boot_modules feature]

	append boot_modules {
		nitpicker
		report_rom
		floating_window_layouter
		decorator
		wm
		pointer
	}

	return $boot_modules
}


#
# tar archive
#

proc create_qt5_fs_tar_archive { app_name qt_modules } {

	exec rm -rf bin/qt5_fs/${app_name}/qt

	if { [lsearch ${qt_modules} "gui"] != -1 } {
		# add fonts
		exec mkdir -p bin/qt5_fs/${app_name}/qt/lib
		exec ln -sf [pwd]/bin/qt5_fs/qt/lib/fonts bin/qt5_fs/${app_name}/qt/lib/fonts
	}

	if { [lsearch ${qt_modules} "quick"] != -1 } {
		# add QtQuick plugins
		exec mkdir -p bin/qt5_fs/${app_name}/qt
		exec ln -sf [pwd]/bin/qt5_fs/qt/qml bin/qt5_fs/${app_name}/qt/qml
	}

	exec tar chf bin/qt5_fs_${app_name}.tar -C bin/qt5_fs/${app_name} .
}
