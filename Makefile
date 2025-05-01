DIST_DIR = dist
SRC_DIR = ./src
DLL_DIR = ./src/dll

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(DIST_DIR)/%.o, $(notdir $(SRC))) $(SRC_DIR)/MinHook/libMinHook.a

TARGET = sky-key.dll
BIN_TARGET = $(DIST_DIR)/$(TARGET)

CC = gcc
PARAM = -Wall -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -static -flto -s

$(BIN_TARGET): $(OBJ)
	$(CC) $(PARAM) $(OBJ) -shared -o $@ -lpsapi -lws2_32

$(DIST_DIR)/dllmain.o: $(SRC_DIR)/dllmain.c
	$(CC) $(PARAM) -c $(SRC_DIR)/dllmain.c -DBUILD_DLL -o $(DIST_DIR)/dllmain.o

$(DIST_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(PARAM) -c $< -o $@ -lpsapi

$(SRC_DIR)/MinHook/libMinHook.a:
	make -C $(SRC_DIR)/MinHook/ libMinHook.a

clean:
	del .\dist\*.o
	del .\dist\main.dll
	make -C $(SRC_DIR)/MinHook/ clean