# Race test history

`collect_stage4.sh` overwrites the race files on every run, so this folder keeps the results of earlier sessions. All sessions below used the final test design: each process opens the device once, loops on `pread`/`pwrite`, and all four start together at a pipe "starting gate"; 2 writers and 2 readers, 200,000 iterations each (400,000 reads per run). VM: kernel 7.0.0-38-generic, 2 vCPUs.

| Session | Code version | Lock on (torn reads, 3 runs) | Lock off (torn reads, 3 runs) | Where tears split (offset mod 64) |
| --- | --- | --- | --- | --- |
| 1 | `c666a47` | 0, 0, 0 | 7,647, 1, 3 | not recorded yet |
| 2 | `f6127e4` | 0, 0, 0 | 3, 0, 1,223 | 32 (5), 8 (1,220), 0 (1) |
| 3 | `e94b5fb` | 0, 0, 0 | 1, 0, 3 | 32 (4) |

- Session 1's full output is in `session1_*.txt` (copied from commit `1cb875e`, where it was first committed).
- Session 2's numbers come from the summary the script printed; its files were overwritten before they were copied off the VM.
- Session 3 is the current set of `stage4_test_race_*.txt` files in the parent folder.
- An earlier manual run (before the starting gate was added, 500,000 iterations) found 1,458 torn reads in 1,000,000.

**Totals:** with the lock on, 0 torn reads in 3,600,000 reads (9 runs). With the lock off, torn reads in 7 of 9 runs, from 0 to 7,647 per run.

**Where tears split:** `kbuf` is at `0x…7060`, 32 bytes into a 64-byte cache line, so buffer offsets 32, 96, 160, … are `kbuf`'s cache-line boundaries. All 7 tears in sessions 2 and 3 outside the one large run split exactly there (plus 2 of the large run's tears). The large session 2 run (1,223 tears) split almost entirely at offset 8, which is not a `kbuf` cache-line boundary; reader buffers are 16-byte aligned (they started at byte 0, 32, or 48 of a line in session 3), so it isn't a reader cache-line boundary either. That cluster is unexplained.
