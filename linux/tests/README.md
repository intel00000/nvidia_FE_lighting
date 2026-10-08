# felight tests

```bash
make -C linux check                             # build the test copies and run every suite
make -C linux check CHECK_ARGS='-v -k service'  # list each test; run those with "service" in their name
python3 linux/tests/run.py -h                   # suites and options
```

The tests need Python 3 and, for the boot service tests, bubblewrap (`bwrap`). They need no GPU and no NVIDIA driver. Without `bwrap`, those tests are skipped with a message; `--strict` makes a skip fail the run, as CI does.

## Safety

The tests never touch a real card:

- They run only test copies of felight, which `make check` links with `shim/wrap.c` and `-Wl,--wrap=dlopen`. Every `dlopen` then loads `$MOCK_NVAPI_LIB`, aborts when it is unset, and aborts when the library is not the mock. The runner refuses any binary whose `nm` lacks `__wrap_dlopen`.
- `felight service` runs only inside a bubblewrap sandbox, where `/etc` and `/usr/local/lib` are directories of the test's work directory, everything else is read-only, and `systemctl` is a script that logs its arguments.
- Each run gets a fresh work directory under `build/tests/work` as its `HOME`, and none of the user's environment.

Test copies also skip `usleep` and `sleep`; `sleep` calls are logged instead.

## Layout

| Path | What it is |
| --- | --- |
| `mock/` | the mock `libnvidia-api.so.1`: serves the cards of a rig file, saves zone writes back to it, logs calls |
| `shim/wrap.c` | linked into test copies of felight |
| `rigs/` | the mock's cards, one scenario per file |
| `harness/` | the runner: cases, running test copies, the sandbox, normalizing output |
| `suites/` | the tests: `smoke` checks the test machinery |

## Rig files

The mock reads the file in `$MOCK_NVAPI_STATE`, and rewrites it in a canonical form after each zone write. Each call to `NvAPI_Initialize` and `NvAPI_GPU_ClientIllumZonesSetControl` is appended to `$MOCK_NVAPI_LOG`.

```text
# comment
option unresolved=NvAPI_GPU_GetUUID initialize_failures=2 fail=enumerate
order 1 0
card uuid=GPU-... pci=BUS,DEVICE,SUBSYSTEM,REVISION,EXT info=RT,TENSOR,EXTERNAL fail=name mask=0x3 name=NVIDIA GeForce RTX 5090
zone type=rgb loc=gpu_top dev=0 prov=0 mode=manual rgbw=255,32,0,0 br=100 drgbw=255,255,255,0 dbr=100 fault=set
```

- `option`: `unresolved` lists functions `nvapi_QueryInterface` does not resolve; `initialize_failures` is a number of failing `NvAPI_Initialize` calls, or `always`; `fail=enumerate` makes `NvAPI_EnumPhysicalGPUs` fail.
- `order`: the cards the driver enumerates, by their position in the file; without it, all cards in file order.
- `card`: `uuid` is a UUID, `unsupported` (the default) or `zero`; without `pci` or `info`, those calls return `NVAPI_NOT_SUPPORTED`. `fail` lists calls that fail: `name`, `info` (a card without lighting), `control`, `devices`. `info_zones` and `control_zones` report fewer zones than the card has. `name` comes last and runs to the end of the line, with `\\` and `\xNN` escapes.
- `zone`: `type` is `rgb`, `rgbw`, `single_color`, `color_fixed`, `invalid` or a number. `mode` is `manual` or `piecewise`; a piecewise zone has `ep0` and `ep1` (`R,G,B,W,BRIGHTNESS`) and `pw` (`CYCLE,GROUPS,RISE,FALL,A,B,IDLE,PHASE`). Keys starting with `d` set the stored defaults. `fault=set` rejects writes that change the zone; `fault=stuck` accepts them and keeps the old values.
