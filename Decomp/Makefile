CXX ?= c++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -Werror
OUT ?= ../build/decomp
GAME ?= $(HOME)/.local/share/Steam/steamapps/common/Riff Racer/Game.exe

all: $(OUT)/reriff-match

$(OUT)/draft-assembly: draft_assembly.cpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) $< -o $@

$(OUT)/reriff-match: main.cpp matching.hpp build_evidence.hpp native_blocks.hpp coverage.hpp ../NativeLinux/native_common.hpp ../NativeLinux/archive.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) main.cpp -o $@ -lz -lcrypto

$(OUT)/test-matching: test_matching.cpp matching.hpp build_evidence.hpp native_blocks.hpp coverage.hpp ../NativeLinux/native_common.hpp ../NativeLinux/archive.hpp
	mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test_matching.cpp -o $@ -lz -lcrypto

test: $(OUT)/test-matching
	$(OUT)/test-matching

$(OUT)/exe_entry.bin: native/exe_entry.S
	mkdir -p $(OUT)
	as --32 native/exe_entry.S -o $(OUT)/exe_entry.o
	objcopy -O binary --only-section=.text $(OUT)/exe_entry.o $@

test-entry: $(OUT)/reriff-match $(OUT)/exe_entry.bin
	$(OUT)/reriff-match entry-check "$(GAME)" $(OUT)/exe_entry.bin $(OUT)/entry-comparison.json

.PHONY: all test test-entry

$(OUT)/security_cookie.bin: native/security_cookie.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/security_cookie.o
	objcopy -O binary --only-section=.text $(OUT)/security_cookie.o $@

$(OUT)/crt_entry.bin: native/crt_entry.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/crt_entry.o
	ld -m elf_i386 -Ttext=0x008F8A84 -e original_crt_entry --defsym=original_security_cookie_init=0x008F9B78 --defsym=original_crt_startup_common=0x008F8A8E $(OUT)/crt_entry.o -o $(OUT)/crt_entry.elf
	objcopy -O binary --only-section=.text $(OUT)/crt_entry.elf $@

$(OUT)/crt_startup.bin: native/crt_startup.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/crt_startup.o
	ld -m elf_i386 -Ttext=0x008F8A8E -e original_crt_startup_common $(OUT)/crt_startup.o -o $(OUT)/crt_startup.elf
	objcopy -O binary --only-section=.text $(OUT)/crt_startup.elf $@

$(OUT)/crt_seh.bin: native/crt_seh.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/crt_seh.o
	objcopy -O binary --only-section=.text $(OUT)/crt_seh.o $@

$(OUT)/crt_scope.bin: native/crt_scope.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/crt_scope.o
	objcopy -O binary --only-section=.rodata $(OUT)/crt_scope.o $@

test-startup: $(OUT)/reriff-match $(OUT)/security_cookie.bin $(OUT)/crt_entry.bin $(OUT)/crt_startup.bin $(OUT)/crt_seh.bin $(OUT)/crt_scope.bin
	$(OUT)/reriff-match native-check "$(GAME)" security-cookie $(OUT)/security_cookie.bin $(OUT)/security-cookie-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" crt-entry $(OUT)/crt_entry.bin $(OUT)/crt-entry-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" crt-startup $(OUT)/crt_startup.bin $(OUT)/crt-startup-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" crt-seh $(OUT)/crt_seh.bin $(OUT)/crt-seh-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" crt-scope $(OUT)/crt_scope.bin $(OUT)/crt-scope-comparison.json

.PHONY: test-startup

$(OUT)/application_entry.bin: native/application_entry.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/application_entry.o
	ld -m elf_i386 -Ttext=0x005CFC90 -e original_application_entry $(OUT)/application_entry.o -o $(OUT)/application_entry.elf
	objcopy -O binary --only-section=.text $(OUT)/application_entry.elf $@

test-application: $(OUT)/reriff-match $(OUT)/application_entry.bin
	$(OUT)/reriff-match native-check "$(GAME)" application-entry $(OUT)/application_entry.bin $(OUT)/application-entry-comparison.json

.PHONY: test-application

$(OUT)/startup_objects.elf: native/startup_objects.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/startup_objects.o
	ld -m elf_i386 -e object_5cfb10 --section-start=.object_5cfb10=0x005CFB10 --section-start=.object_5cfb60=0x005CFB60 --section-start=.object_5cfbe0=0x005CFBE0 --section-start=.object_5cfc60=0x005CFC60 $(OUT)/startup_objects.o -o $@

$(OUT)/object_%.bin: $(OUT)/startup_objects.elf
	objcopy -O binary --only-section=.object_$* $< $@

test-objects: $(OUT)/reriff-match $(OUT)/object_5cfb10.bin $(OUT)/object_5cfb60.bin $(OUT)/object_5cfbe0.bin $(OUT)/object_5cfc60.bin
	$(OUT)/reriff-match native-check "$(GAME)" object-5cfb10 $(OUT)/object_5cfb10.bin $(OUT)/object-5cfb10-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" object-5cfb60 $(OUT)/object_5cfb60.bin $(OUT)/object-5cfb60-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" object-5cfbe0 $(OUT)/object_5cfbe0.bin $(OUT)/object-5cfbe0-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" object-5cfc60 $(OUT)/object_5cfc60.bin $(OUT)/object-5cfc60-comparison.json

.PHONY: test-objects

$(OUT)/message_pump.bin: native/message_pump.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/message_pump.o
	objcopy -O binary --only-section=.text $(OUT)/message_pump.o $@

$(OUT)/worker_thread.elf: native/worker_thread.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/worker_thread.o
	ld -m elf_i386 -e original_worker_delete --section-start=.worker_delete=0x0048DB80 --section-start=.worker_destroy=0x0048DBB0 --section-start=.worker_start=0x0048DBC0 --section-start=.worker_dispatch=0x0048DC50 $(OUT)/worker_thread.o -o $@

$(OUT)/worker_%.bin: $(OUT)/worker_thread.elf
	objcopy -O binary --only-section=.worker_$* $< $@

test-worker: $(OUT)/reriff-match $(OUT)/message_pump.bin $(OUT)/worker_delete.bin $(OUT)/worker_destroy.bin $(OUT)/worker_start.bin $(OUT)/worker_dispatch.bin
	$(OUT)/reriff-match native-check "$(GAME)" message-pump $(OUT)/message_pump.bin $(OUT)/message-pump-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" worker-delete $(OUT)/worker_delete.bin $(OUT)/worker-delete-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" worker-destroy $(OUT)/worker_destroy.bin $(OUT)/worker-destroy-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" worker-start $(OUT)/worker_start.bin $(OUT)/worker-start-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" worker-dispatch $(OUT)/worker_dispatch.bin $(OUT)/worker-dispatch-comparison.json

.PHONY: test-worker

$(OUT)/worker_request.bin: native/worker_request.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/worker_request.o
	objcopy -O binary --only-section=.text $(OUT)/worker_request.o $@

test-request: $(OUT)/reriff-match $(OUT)/worker_request.bin
	$(OUT)/reriff-match native-check "$(GAME)" worker-request $(OUT)/worker_request.bin $(OUT)/worker-request-comparison.json

.PHONY: test-request

$(OUT)/request_dispatch.bin: native/request_dispatch.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/request_dispatch.o
	ld -m elf_i386 -Ttext=0x004A1900 -e original_request_dispatch $(OUT)/request_dispatch.o -o $(OUT)/request_dispatch.elf
	objcopy -O binary --only-section=.text $(OUT)/request_dispatch.elf $@

test-dispatch: $(OUT)/reriff-match $(OUT)/request_dispatch.bin
	$(OUT)/reriff-match native-check "$(GAME)" request-dispatch $(OUT)/request_dispatch.bin $(OUT)/request-dispatch-comparison.json

.PHONY: test-dispatch

$(OUT)/request_cleanup.bin: native/request_cleanup.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/request_cleanup.o
	objcopy -O binary --only-section=.text $(OUT)/request_cleanup.o $@

test-cleanup: $(OUT)/reriff-match $(OUT)/request_cleanup.bin
	$(OUT)/reriff-match native-check "$(GAME)" request-cleanup $(OUT)/request_cleanup.bin $(OUT)/request-cleanup-comparison.json

test-recovered: test test-entry test-startup test-application test-objects test-worker test-request test-dispatch test-cleanup test-case3 test-case2 test-window test-secondary-window

.PHONY: test-cleanup test-recovered

$(OUT)/request_case3.bin: native/request_case3.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/request_case3.o
	ld -m elf_i386 -Ttext=0x004A2130 -e original_request_case3 $(OUT)/request_case3.o -o $(OUT)/request_case3.elf
	objcopy -O binary --only-section=.text $(OUT)/request_case3.elf $@

test-case3: $(OUT)/reriff-match $(OUT)/request_case3.bin
	$(OUT)/reriff-match native-check "$(GAME)" request-case3 $(OUT)/request_case3.bin $(OUT)/request-case3-comparison.json

.PHONY: test-case3

$(OUT)/request_case2.bin: native/request_case2.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/request_case2.o
	ld -m elf_i386 -Ttext=0x004A1CD0 -e original_request_case2 $(OUT)/request_case2.o -o $(OUT)/request_case2.elf
	objcopy -O binary --only-section=.text $(OUT)/request_case2.elf $@

test-case2: $(OUT)/reriff-match $(OUT)/request_case2.bin
	$(OUT)/reriff-match native-check "$(GAME)" request-case2 $(OUT)/request_case2.bin $(OUT)/request-case2-comparison.json

.PHONY: test-case2

$(OUT)/window_mode.bin: native/window_mode.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/window_mode.o
	ld -m elf_i386 -Ttext=0x0048CB30 -e original_window_mode $(OUT)/window_mode.o -o $(OUT)/window_mode.elf
	objcopy -O binary --only-section=.text $(OUT)/window_mode.elf $@

$(OUT)/cookie_check.bin: native/cookie_check.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/cookie_check.o
	ld -m elf_i386 -Ttext=0x008F8958 -e original_cookie_check $(OUT)/cookie_check.o -o $(OUT)/cookie_check.elf
	objcopy -O binary --only-section=.text $(OUT)/cookie_check.elf $@

test-window: $(OUT)/reriff-match $(OUT)/window_mode.bin $(OUT)/cookie_check.bin
	$(OUT)/reriff-match native-check "$(GAME)" window-mode $(OUT)/window_mode.bin $(OUT)/window-mode-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" cookie-check $(OUT)/cookie_check.bin $(OUT)/cookie-check-comparison.json

.PHONY: test-window

$(OUT)/secondary_window.bin: native/secondary_window.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/secondary_window.o
	ld -m elf_i386 -Ttext=0x0048CC70 -e original_secondary_window $(OUT)/secondary_window.o -o $(OUT)/secondary_window.elf
	objcopy -O binary --only-section=.text $(OUT)/secondary_window.elf $@

test-secondary-window: $(OUT)/reriff-match $(OUT)/secondary_window.bin
	$(OUT)/reriff-match native-check "$(GAME)" secondary-window $(OUT)/secondary_window.bin $(OUT)/secondary-window-comparison.json

.PHONY: test-secondary-window

$(OUT)/window_frame.bin: native/window_frame.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/window_frame.o
	ld -m elf_i386 -Ttext=0x0048CAB0 -e original_window_frame $(OUT)/window_frame.o -o $(OUT)/window_frame.elf
	objcopy -O binary --only-section=.text $(OUT)/window_frame.elf $@

test-window-frame: $(OUT)/reriff-match $(OUT)/window_frame.bin
	$(OUT)/reriff-match native-check "$(GAME)" window-frame $(OUT)/window_frame.bin $(OUT)/window-frame-comparison.json

test-recovered: test-window-frame
.PHONY: test-window-frame

$(OUT)/viewport_size.bin: native/viewport_size.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/viewport_size.o
	ld -m elf_i386 -Ttext=0x004A22E0 -e original_viewport_size $(OUT)/viewport_size.o -o $(OUT)/viewport_size.elf
	objcopy -O binary --only-section=.text $(OUT)/viewport_size.elf $@

$(OUT)/fit_dimensions.bin: native/fit_dimensions.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/fit_dimensions.o
	ld -m elf_i386 -Ttext=0x004A1C30 -e original_fit_dimensions $(OUT)/fit_dimensions.o -o $(OUT)/fit_dimensions.elf
	objcopy -O binary --only-section=.text $(OUT)/fit_dimensions.elf $@

test-viewport: $(OUT)/reriff-match $(OUT)/viewport_size.bin $(OUT)/fit_dimensions.bin
	$(OUT)/reriff-match native-check "$(GAME)" viewport-size $(OUT)/viewport_size.bin $(OUT)/viewport-size-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" fit-dimensions $(OUT)/fit_dimensions.bin $(OUT)/fit-dimensions-comparison.json

test-recovered: test-viewport
.PHONY: test-viewport

$(OUT)/floor_float.bin: native/floor_float.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/floor_float.o
	ld -m elf_i386 -Ttext=0x00426D20 -e original_floor_float $(OUT)/floor_float.o -o $(OUT)/floor_float.elf
	objcopy -O binary --only-section=.text $(OUT)/floor_float.elf $@

$(OUT)/remove_registered.bin: native/remove_registered.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/remove_registered.o
	objcopy -O binary --only-section=.text $(OUT)/remove_registered.o $@

test-native-helpers: $(OUT)/reriff-match $(OUT)/floor_float.bin $(OUT)/remove_registered.bin
	$(OUT)/reriff-match native-check "$(GAME)" floor-float $(OUT)/floor_float.bin $(OUT)/floor-float-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" remove-registered $(OUT)/remove_registered.bin $(OUT)/remove-registered-comparison.json

test-recovered: test-native-helpers
.PHONY: test-native-helpers

$(OUT)/load_graphics.bin: native/load_graphics.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/load_graphics.o
	ld -m elf_i386 -Ttext=0x004A2460 -e original_load_graphics $(OUT)/load_graphics.o -o $(OUT)/load_graphics.elf
	objcopy -O binary --only-section=.text $(OUT)/load_graphics.elf $@

test-load-graphics: $(OUT)/reriff-match $(OUT)/load_graphics.bin
	$(OUT)/reriff-match native-check "$(GAME)" load-graphics $(OUT)/load_graphics.bin $(OUT)/load-graphics-comparison.json

test-recovered: test-load-graphics
.PHONY: test-load-graphics

$(OUT)/resize_resources.bin: native/resize_resources.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/resize_resources.o
	ld -m elf_i386 -Ttext=0x00528F80 -e original_resize_resources $(OUT)/resize_resources.o -o $(OUT)/resize_resources.elf
	objcopy -O binary --only-section=.text $(OUT)/resize_resources.elf $@

test-resize-resources: $(OUT)/reriff-match $(OUT)/resize_resources.bin
	$(OUT)/reriff-match native-check "$(GAME)" resize-resources $(OUT)/resize_resources.bin $(OUT)/resize-resources-comparison.json

test-recovered: test-resize-resources
.PHONY: test-resize-resources

$(OUT)/save_graphics.bin: native/save_graphics.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_graphics.o
	ld -m elf_i386 -Ttext=0x004A2640 -e original_save_graphics $(OUT)/save_graphics.o -o $(OUT)/save_graphics.elf
	objcopy -O binary --only-section=.text $(OUT)/save_graphics.elf $@

test-save-graphics: $(OUT)/reriff-match $(OUT)/save_graphics.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-graphics $(OUT)/save_graphics.bin $(OUT)/save-graphics-comparison.json

test-recovered: test-save-graphics
.PHONY: test-save-graphics

$(OUT)/save_device_ctor.bin: native/save_device_ctor.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_ctor.o
	objcopy -O binary --only-section=.text $(OUT)/save_device_ctor.o $@

test-save-device: $(OUT)/reriff-match $(OUT)/save_device_ctor.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-ctor $(OUT)/save_device_ctor.bin $(OUT)/save-device-ctor-comparison.json

test-recovered: test-save-device
.PHONY: test-save-device

$(OUT)/save_device_close.bin: native/save_device_close.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_close.o
	objcopy -O binary --only-section=.text $(OUT)/save_device_close.o $@

$(OUT)/save_device_delete.bin: native/save_device_delete.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_delete.o
	ld -m elf_i386 -Ttext=0x00535CC0 -e original_save_device_delete $(OUT)/save_device_delete.o -o $(OUT)/save_device_delete.elf
	objcopy -O binary --only-section=.text $(OUT)/save_device_delete.elf $@

test-save-device-operations: $(OUT)/reriff-match $(OUT)/save_device_close.bin $(OUT)/save_device_delete.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-close $(OUT)/save_device_close.bin $(OUT)/save-device-close-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" save-device-delete $(OUT)/save_device_delete.bin $(OUT)/save-device-delete-comparison.json

test-recovered: test-save-device-operations
.PHONY: test-save-device-operations

$(OUT)/save_device_io.o: native/save_device_io.S
	mkdir -p $(OUT)
	as --32 $< -o $@

$(OUT)/save_device_read.bin: $(OUT)/save_device_io.o
	objcopy -O binary --only-section=.read $< $@

$(OUT)/save_device_write.bin: $(OUT)/save_device_io.o
	objcopy -O binary --only-section=.write $< $@

test-save-device-io: $(OUT)/reriff-match $(OUT)/save_device_read.bin $(OUT)/save_device_write.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-read $(OUT)/save_device_read.bin $(OUT)/save-device-read-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" save-device-write $(OUT)/save_device_write.bin $(OUT)/save-device-write-comparison.json

test-recovered: test-save-device-io
.PHONY: test-save-device-io

$(OUT)/save_device_open.bin: native/save_device_open.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_open.o
	ld -m elf_i386 -Ttext=0x00535B10 -e original_save_device_open $(OUT)/save_device_open.o -o $(OUT)/save_device_open.elf
	objcopy -O binary --only-section=.text $(OUT)/save_device_open.elf $@

test-save-device-open: $(OUT)/reriff-match $(OUT)/save_device_open.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-open $(OUT)/save_device_open.bin $(OUT)/save-device-open-comparison.json

test-recovered: test-save-device-open
.PHONY: test-save-device-open

coverage: test-recovered
	$(OUT)/reriff-match coverage "$(GAME)" $(OUT) $(OUT)/coverage.json

.PHONY: coverage

$(OUT)/save_device_exists.bin: native/save_device_exists.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_exists.o
	ld -m elf_i386 -Ttext=0x00535810 -e original_save_device_exists $(OUT)/save_device_exists.o -o $(OUT)/save_device_exists.elf
	objcopy -O binary --only-section=.text $(OUT)/save_device_exists.elf $@

$(OUT)/save_device_create.bin: native/save_device_create.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_create.o
	ld -m elf_i386 -Ttext=0x005358F0 -e original_save_device_create $(OUT)/save_device_create.o -o $(OUT)/save_device_create.elf
	objcopy -O binary --only-section=.text $(OUT)/save_device_create.elf $@

test-save-device-create: $(OUT)/reriff-match $(OUT)/save_device_exists.bin $(OUT)/save_device_create.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-exists $(OUT)/save_device_exists.bin $(OUT)/save-device-exists-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" save-device-create $(OUT)/save_device_create.bin $(OUT)/save-device-create-comparison.json

test-recovered: test-save-device-create
.PHONY: test-save-device-create

test-draft: $(OUT)/draft-assembly
	bash test-draft.sh $(OUT)/draft-assembly

.PHONY: test-draft

$(OUT)/save_device_size.bin: native/save_device_size.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/save_device_size.o
	objcopy -O binary --only-section=.text $(OUT)/save_device_size.o $@

test-save-device-size: $(OUT)/reriff-match $(OUT)/save_device_size.bin
	$(OUT)/reriff-match native-check "$(GAME)" save-device-size $(OUT)/save_device_size.bin $(OUT)/save-device-size-comparison.json

test-recovered: test-save-device-size
.PHONY: test-save-device-size

$(OUT)/application_worker.bin: native/application_worker.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/application_worker.o
	ld -m elf_i386 -Ttext=0x005282E0 -e original_application_worker $(OUT)/application_worker.o -o $(OUT)/application_worker.elf
	objcopy -O binary --only-section=.text $(OUT)/application_worker.elf $@

test-application-worker: $(OUT)/reriff-match $(OUT)/application_worker.bin
	$(OUT)/reriff-match native-check "$(GAME)" application-worker $(OUT)/application_worker.bin $(OUT)/application-worker-comparison.json

test-recovered: test-application-worker
.PHONY: test-application-worker

$(OUT)/worker_queue.bin: native/worker_queue.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/worker_queue.o
	ld -m elf_i386 -Ttext=0x00528930 -e original_worker_queue $(OUT)/worker_queue.o -o $(OUT)/worker_queue.elf
	objcopy -O binary --only-section=.text $(OUT)/worker_queue.elf $@

test-worker-queue: $(OUT)/reriff-match $(OUT)/worker_queue.bin
	$(OUT)/reriff-match native-check "$(GAME)" worker-queue $(OUT)/worker_queue.bin $(OUT)/worker-queue-comparison.json

test-recovered: test-worker-queue
.PHONY: test-worker-queue

$(OUT)/application_frame.bin: native/application_frame.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/application_frame.o
	ld -m elf_i386 -Ttext=0x004A0AF0 -e original_application_frame $(OUT)/application_frame.o -o $(OUT)/application_frame.elf
	objcopy -O binary --only-section=.text $(OUT)/application_frame.elf $@

test-application-frame: $(OUT)/reriff-match $(OUT)/application_frame.bin
	$(OUT)/reriff-match native-check "$(GAME)" application-frame $(OUT)/application_frame.bin $(OUT)/application-frame-comparison.json

test-recovered: test-application-frame
.PHONY: test-application-frame

$(OUT)/device_release.bin: native/device_release.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/device_release.o
	ld -m elf_i386 -Ttext=0x004A6330 -e original_device_release $(OUT)/device_release.o -o $(OUT)/device_release.elf
	objcopy -O binary --only-section=.text $(OUT)/device_release.elf $@

$(OUT)/device_restore.bin: native/device_restore.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/device_restore.o
	ld -m elf_i386 -Ttext=0x004A6560 -e original_device_restore $(OUT)/device_restore.o -o $(OUT)/device_restore.elf
	objcopy -O binary --only-section=.text $(OUT)/device_restore.elf $@

test-device-recovery: $(OUT)/reriff-match $(OUT)/device_release.bin $(OUT)/device_restore.bin
	$(OUT)/reriff-match native-check "$(GAME)" device-release $(OUT)/device_release.bin $(OUT)/device-release-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" device-restore $(OUT)/device_restore.bin $(OUT)/device-restore-comparison.json

test-recovered: test-device-recovery
.PHONY: test-device-recovery

$(OUT)/query_create.bin: native/query_create.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/query_create.o
	ld -m elf_i386 -Ttext=0x004A4C50 -e original_query_create $(OUT)/query_create.o -o $(OUT)/query_create.elf
	objcopy -O binary --only-section=.text $(OUT)/query_create.elf $@

$(OUT)/query_release.bin: native/query_release.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/query_release.o
	objcopy -O binary --only-section=.text $(OUT)/query_release.o $@

test-query-resources: $(OUT)/reriff-match $(OUT)/query_create.bin $(OUT)/query_release.bin
	$(OUT)/reriff-match native-check "$(GAME)" query-create $(OUT)/query_create.bin $(OUT)/query-create-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" query-release $(OUT)/query_release.bin $(OUT)/query-release-comparison.json

test-recovered: test-query-resources
.PHONY: test-query-resources

$(OUT)/colour_targets.bin: native/colour_targets.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/colour_targets.o
	ld -m elf_i386 -Ttext=0x4a5240 -e original_colour_targets $(OUT)/colour_targets.o -o $(OUT)/colour_targets.elf
	objcopy -O binary --only-section=.text $(OUT)/colour_targets.elf $@

$(OUT)/effect_target.bin: native/effect_target.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/effect_target.o
	ld -m elf_i386 -Ttext=0x4a5530 -e original_effect_target $(OUT)/effect_target.o -o $(OUT)/effect_target.elf
	objcopy -O binary --only-section=.text $(OUT)/effect_target.elf $@

$(OUT)/aux_target_a.bin: native/aux_target_a.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/aux_target_a.o
	ld -m elf_i386 -Ttext=0x4a56c0 -e original_aux_target_a $(OUT)/aux_target_a.o -o $(OUT)/aux_target_a.elf
	objcopy -O binary --only-section=.text $(OUT)/aux_target_a.elf $@

$(OUT)/aux_target_b.bin: native/aux_target_b.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/aux_target_b.o
	ld -m elf_i386 -Ttext=0x4a5850 -e original_aux_target_b $(OUT)/aux_target_b.o -o $(OUT)/aux_target_b.elf
	objcopy -O binary --only-section=.text $(OUT)/aux_target_b.elf $@

test-target-creation: $(OUT)/reriff-match $(OUT)/colour_targets.bin $(OUT)/effect_target.bin $(OUT)/aux_target_a.bin $(OUT)/aux_target_b.bin
	$(OUT)/reriff-match native-check "$(GAME)" colour-targets $(OUT)/colour_targets.bin $(OUT)/colour-targets-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" effect-target $(OUT)/effect_target.bin $(OUT)/effect-target-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" aux-target-a $(OUT)/aux_target_a.bin $(OUT)/aux-target-a-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" aux-target-b $(OUT)/aux_target_b.bin $(OUT)/aux-target-b-comparison.json

test-recovered: test-target-creation
.PHONY: test-target-creation

$(OUT)/device_defaults.bin: native/device_defaults.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/device_defaults.o
	objcopy -O binary --only-section=.text $(OUT)/device_defaults.o $@

test-device-defaults: $(OUT)/reriff-match $(OUT)/device_defaults.bin
	$(OUT)/reriff-match native-check "$(GAME)" device-defaults $(OUT)/device_defaults.bin $(OUT)/device-defaults-comparison.json

test-recovered: test-device-defaults
.PHONY: test-device-defaults

$(OUT)/buffer_allocate.bin: native/buffer_allocate.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/buffer_allocate.o
	objcopy -O binary --only-section=.text $(OUT)/buffer_allocate.o $@

test-buffer-allocate: $(OUT)/reriff-match $(OUT)/buffer_allocate.bin
	$(OUT)/reriff-match native-check "$(GAME)" buffer-allocate $(OUT)/buffer_allocate.bin $(OUT)/buffer-allocate-comparison.json

test-recovered: test-buffer-allocate
.PHONY: test-buffer-allocate

$(OUT)/device_create.bin: native/device_create.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/device_create.o
	ld -m elf_i386 -Ttext=0x004a3630 -e original_device_create $(OUT)/device_create.o -o $(OUT)/device_create.elf
	objcopy -O binary --only-section=.text $(OUT)/device_create.elf $@

test-device-create: $(OUT)/reriff-match $(OUT)/device_create.bin
	$(OUT)/reriff-match native-check "$(GAME)" device-create $(OUT)/device_create.bin $(OUT)/device-create-comparison.json

test-recovered: test-device-create
.PHONY: test-device-create

$(OUT)/device_destroy.bin: native/device_destroy.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/device_destroy.o
	ld -m elf_i386 -Ttext=0x004a5c40 -e original_device_destroy $(OUT)/device_destroy.o -o $(OUT)/device_destroy.elf
	objcopy -O binary --only-section=.text $(OUT)/device_destroy.elf $@

test-device-destroy: $(OUT)/reriff-match $(OUT)/device_destroy.bin
	$(OUT)/reriff-match native-check "$(GAME)" device-destroy $(OUT)/device_destroy.bin $(OUT)/device-destroy-comparison.json

test-recovered: test-device-destroy
.PHONY: test-device-destroy

$(OUT)/presentation_parameters.bin: native/presentation_parameters.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/presentation_parameters.o
	ld -m elf_i386 -Ttext=0x4a33f0 -e original_presentation_parameters $(OUT)/presentation_parameters.o -o $(OUT)/presentation_parameters.elf
	objcopy -O binary --only-section=.text $(OUT)/presentation_parameters.elf $@

$(OUT)/vertex_declarations.bin: native/vertex_declarations.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/vertex_declarations.o
	ld -m elf_i386 -Ttext=0x4a3a40 -e original_vertex_declarations $(OUT)/vertex_declarations.o -o $(OUT)/vertex_declarations.elf
	objcopy -O binary --only-section=.text $(OUT)/vertex_declarations.elf $@

test-presentation-resources: $(OUT)/reriff-match $(OUT)/presentation_parameters.bin $(OUT)/vertex_declarations.bin
	$(OUT)/reriff-match native-check "$(GAME)" presentation-parameters $(OUT)/presentation_parameters.bin $(OUT)/presentation-parameters-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" vertex-declarations $(OUT)/vertex_declarations.bin $(OUT)/vertex-declarations-comparison.json

test-recovered: test-presentation-resources
.PHONY: test-presentation-resources

$(OUT)/graphics_startup.bin: native/graphics_startup.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/graphics_startup.o
	ld -m elf_i386 -Ttext=0x4a0480 -e original_graphics_startup $(OUT)/graphics_startup.o -o $(OUT)/graphics_startup.elf
	objcopy -O binary --only-section=.text $(OUT)/graphics_startup.elf $@

$(OUT)/graphics_shutdown.bin: native/graphics_shutdown.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/graphics_shutdown.o
	ld -m elf_i386 -Ttext=0x4a08e0 -e original_graphics_shutdown $(OUT)/graphics_shutdown.o -o $(OUT)/graphics_shutdown.elf
	objcopy -O binary --only-section=.text $(OUT)/graphics_shutdown.elf $@

test-graphics-lifetime: $(OUT)/reriff-match $(OUT)/graphics_startup.bin $(OUT)/graphics_shutdown.bin
	$(OUT)/reriff-match native-check "$(GAME)" graphics-startup $(OUT)/graphics_startup.bin $(OUT)/graphics-startup-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" graphics-shutdown $(OUT)/graphics_shutdown.bin $(OUT)/graphics-shutdown-comparison.json

test-recovered: test-graphics-lifetime
.PHONY: test-graphics-lifetime

$(OUT)/graphics_capabilities.bin: native/graphics_capabilities.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/graphics_capabilities.o
	ld -m elf_i386 -Ttext=0x4a2be0 -e original_graphics_capabilities $(OUT)/graphics_capabilities.o -o $(OUT)/graphics_capabilities.elf
	objcopy -O binary --only-section=.text $(OUT)/graphics_capabilities.elf $@

$(OUT)/graphics_options.bin: native/graphics_options.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/graphics_options.o
	ld -m elf_i386 -Ttext=0x4a27d0 -e original_graphics_options $(OUT)/graphics_options.o -o $(OUT)/graphics_options.elf
	objcopy -O binary --only-section=.text $(OUT)/graphics_options.elf $@

test-graphics-capabilities: $(OUT)/reriff-match $(OUT)/graphics_capabilities.bin $(OUT)/graphics_options.bin
	$(OUT)/reriff-match native-check "$(GAME)" graphics-capabilities $(OUT)/graphics_capabilities.bin $(OUT)/graphics-capabilities-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" graphics-options $(OUT)/graphics_options.bin $(OUT)/graphics-options-comparison.json

test-recovered: test-graphics-capabilities
.PHONY: test-graphics-capabilities

$(OUT)/minimum_caps.bin: native/minimum_caps.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/minimum_caps.o
	ld -m elf_i386 -Ttext=0x4a59e0 -e original_minimum_caps $(OUT)/minimum_caps.o -o $(OUT)/minimum_caps.elf
	objcopy -O binary --only-section=.text $(OUT)/minimum_caps.elf $@

$(OUT)/display_modes.bin: native/display_modes.S
	mkdir -p $(OUT)
	as --32 $< -o $(OUT)/display_modes.o
	ld -m elf_i386 -Ttext=0x4a29d0 -e original_display_modes $(OUT)/display_modes.o -o $(OUT)/display_modes.elf
	objcopy -O binary --only-section=.text $(OUT)/display_modes.elf $@

test-capability-helpers: $(OUT)/reriff-match $(OUT)/minimum_caps.bin $(OUT)/display_modes.bin
	$(OUT)/reriff-match native-check "$(GAME)" minimum-caps $(OUT)/minimum_caps.bin $(OUT)/minimum-caps-comparison.json
	$(OUT)/reriff-match native-check "$(GAME)" display-modes $(OUT)/display_modes.bin $(OUT)/display-modes-comparison.json

test-recovered: test-capability-helpers
.PHONY: test-capability-helpers
