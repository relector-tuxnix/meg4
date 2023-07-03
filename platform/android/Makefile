# call Android build (sources listed in app/jni/src/Android.mk)
all: ../sdl/data.c ../../src/data.c
	./gradlew assembleRelease

installRelease: ../sdl/data.c ../../src/data.c
	./gradlew installRelease

installDebug: ../sdl/data.c ../../src/data.c
	./gradlew installDebug

# get generated source files with embedded assets
../sdl/data.c:
	@make --no-print-directory -C ../sdl data.c

../../src/data.c:
	@make --no-print-directory -C ../../src data.c

