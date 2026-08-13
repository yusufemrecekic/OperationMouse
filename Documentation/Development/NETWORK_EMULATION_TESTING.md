# Multiplayer Network Emulation Checks

These are temporary manual PIE checks. They do not change project defaults and do not replace a normal two-player Listen Server test.

## Base PIE setup

1. Open the approved test map.
2. Open the Play drop-down and set **Number of Players** to `2`.
3. Set **Net Mode** to **Play As Listen Server**.
4. Run once with **Enable Network Emulation** disabled. Verify both players can move, use the tested mechanic, see each other, and control only their own Character.

## Temporary latency tests

In the Play multiplayer/network emulation settings:

1. Enable **Network Emulation**.
2. Select **Custom** and **Everyone** so Host and Client are exercised.
3. Apply the same value to incoming and outgoing minimum/maximum latency.
4. Keep packet loss at `0%` for the latency-only passes.

Suggested passes:

| Test | Incoming min/max | Outgoing min/max | Packet loss |
| --- | ---: | ---: | ---: |
| Normal | 0 ms | 0 ms | 0% |
| Approximately 100 ms ping | 50 ms | 50 ms | 0% |
| Approximately 200 ms ping | 100 ms | 100 ms | 0% |
| Small packet loss | 50 ms | 50 ms | 1% |

The approximate ping labels follow UE 5.8's packet-emulation convention: latency is added to traffic directionally, so observed round-trip values can vary. Judge the actual session using `stat net` if an exact measurement matters.

For each pass, check movement visibility, ownership isolation, corrections/teleports, traversal state, and whether either player becomes stuck or desynchronized.

After testing, completely disable **Network Emulation** and run one final normal session. Do not save latency or packet-loss values into project configuration.

