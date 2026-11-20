# Glitch endings, positive endings and continuing play

**Tone:** bargain-bin DS baby-game slapstick, not horror. Corruptions can change controls and challenge timing; arcade prompts clearly signal reversals.

* Tier 0 (0–19): ordinary awkward baby.
* Tier 1 (20–39): brief visual distortion.
* Tier 2 (40–59): every third arcade direction reverses, marked REVERSE.
* Tier 3 (60–79): more challenging 75-frame response window.
* Tier 4 (80–100): clunky face distortion and challenging, signposted arcade twists.

## Good endings — all continue the same save

* **Tiny Diploma Ceremony:** from day 3, maintain high mood and energy with low hunger and chaos.
* **Rattle Arcade Champion:** win at least six arcade challenges, collect $25 and stay in reasonably good shape.
* **Bugs Become Features:** from day 5, safely manage corruption between 40 and 79 with low chaos.
* **Second-Chance Daycare:** from day 10, recover from at least one bad ending to strong care statistics.

Each positive finale grants **$8 and one Favor**, only on first discovery. The ending is added to the 16-bit ending collection without changing the version-1 save layout, meaning preexisting prototype saves remain compatible. The ending screen offers **A or B: CONTINUE PLAYING**. It does not end the program, delete saves, or force credits. A repeated already-seen good finale becomes a normal day instead of infinite reward farming.

## Bad endings — also continue the same save

* **Bottle Subscription Ended:** dangerously hungry.
* **Baby CEO Has Quit:** mood collapses.
* **Mandatory Daycare Reboot:** extreme corruption plus chaos.
* **Unpaid Intern Revolt:** chaos spirals out of control.

Bad endings record their history, worsen corruption, then restore some stats to keep a recovery route possible. Players can reach good endings *after* a bad ending. The day advances and a continue prompt returns to nursery.

## The optional wipe ending

After **three distinct bad endings** or **five total bad endings**, hold L+R in nursery to access an optional *Erase My Memories, Not Your SD Card* reset. Positive endings **do not count** toward this gate. A confirms the warning, then holding L+R and pressing X deliberately erases only `fat:/my_baby_ai.0` and `fat:/my_baby_ai.1`. B cancels. No regular ending, glitch, or arcade failure triggers deletion.

Save writes alternate between two checksummed slots to limit interrupted-write damage. If FAT access is unavailable, play continues in RAM-only mode.

**Status:** the first DS prototype has a single shared arcade challenge behind 16 named cabinets; full bespoke minigames, modern model-based AI, actual emulation benchmarks and real-hardware validation are planned, not yet verified.
