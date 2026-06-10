PIO ?= pio
ENV ?= esp32dev

.PHONY: all build upload monitor clean test check

all: build

build:
	$(PIO) run -e $(ENV)

upload:
	$(PIO) run -e $(ENV) -t upload

monitor:
	$(PIO) device monitor -e $(ENV)

clean:
	$(PIO) run -e $(ENV) -t clean

test:
	$(PIO) test -e $(ENV)

check:
	$(PIO) check -e $(ENV)
