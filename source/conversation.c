#include <limits.h>
#include <string.h>
#include "conversation.h"
#include "dialogue.h"
/* The shared eight-neuron encoder feeds a small trainable style head.
 * Factual topic selection is guarded separately from learned preference.
 * All words below are original authored ROM text, never generated tokens. */
#define VARIANTS 2u
#define REPLIES_PER_TOPIC (CONVERSATION_STYLE_COUNT*VARIANTS)
#define NO_REPLY UINT16_MAX
#define NO_CARE UINT8_MAX
#define QUIET_REQUEST 4u
#define R(t,s,v,text) {text,(uint16_t)((t)*REPLIES_PER_TOPIC+(s)*VARIANTS+(v)),t,s}
static const ConversationReply catalog[]={
 R(CONVERSATION_FEEL_OK,CONVERSATION_WARM,0,
   "I FEEL STEADY AND SAFE RIGHT NOW.\nI'M HAPPY TO HAVE YOU HERE."),
 R(CONVERSATION_FEEL_OK,CONVERSATION_WARM,1,
   "MY LITTLE WORLD FEELS SETTLED.\nWE HAVE ROOM FOR SOMETHING FUN."),
 R(CONVERSATION_FEEL_OK,CONVERSATION_PLAYFUL,0,
   "ALL SYSTEMS GO. ALL TOES PRESENT.\nIT IS A GOOD DAY TO BE SMALL."),
 R(CONVERSATION_FEEL_OK,CONVERSATION_PLAYFUL,1,
   "MY SOCKS AND I HAVE NO COMPLAINTS.\nTEDDY IS STILL REVIEWING HIS."),
 R(CONVERSATION_FEEL_OK,CONVERSATION_CORPORATE,0,
   "DAYCARE OPERATIONS LOOK STABLE.\nTHANK YOU FOR CHECKING ON THE CEO."),
 R(CONVERSATION_FEEL_OK,CONVERSATION_CORPORATE,1,
   "MY CURRENT PLAN IS SIMPLE.\nSTAY COZY. KEEP YOU NEARBY."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_WARM,0,
   "I'M STARTING TO FEEL PECKISH.\nA BOTTLE WOULD BE NICE SOON."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_WARM,1,
   "MY TUMMY IS ASKING A SMALL QUESTION.\nI THINK THE ANSWER IS MILK."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_PLAYFUL,0,
   "MY TUMMY JUST SENT A TINY MEMO.\nIT WAS MOSTLY ABOUT SNACKS."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_PLAYFUL,1,
   "I CHECKED UNDER MY BIB.\nNO EMERGENCY SNACKS WERE FOUND."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_CORPORATE,0,
   "THE LUNCH DEPARTMENT IS STIRRING.\nA BOTTLE MAY JOIN THE AGENDA."),
 R(CONVERSATION_FEEL_HUNGRY,CONVERSATION_CORPORATE,1,
   "THIS IS AN EARLY SNACK FORECAST.\nPLEASE KEEP THE BOTTLE NEARBY."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_WARM,0,
   "I HAVE A LITTLE LESS ENERGY NOW.\nA NAP WOULD FEEL GOOD SOON."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_WARM,1,
   "REST HELPS ME FIND MY BOUNCE AGAIN.\nI DON'T HAVE TO RUSH."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_PLAYFUL,0,
   "MY EYELIDS ARE GETTING HEAVY.\nTHEY DID NOT ASK PERMISSION."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_PLAYFUL,1,
   "MY GIGGLES NEED A SHORT RECHARGE.\nA NAP IS A VERY SMALL ADVENTURE."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_CORPORATE,0,
   "MY ENERGY BUDGET IS RUNNING LOW.\nI AM CONSIDERING PAID NAP LEAVE."),
 R(CONVERSATION_FEEL_TIRED,CONVERSATION_CORPORATE,1,
   "THE PILLOW HAS A FREE APPOINTMENT.\nI MAY ACCEPT ITS GENEROUS OFFER."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_WARM,0,
   "I HAVE ENERGY FOR SOMETHING FUN.\nI LIKE SHARING THESE LITTLE MOMENTS."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_WARM,1,
   "A LITTLE PLAY CAN BRIGHTEN MY DAY.\nYOU MAKE THE NURSERY FEEL LIVELY."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_PLAYFUL,0,
   "MY GIGGLES ARE READY FOR DUTY.\nTEDDY SHOULD BRING HIS BEST SOCKS."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_PLAYFUL,1,
   "I AM PLANNING A VERY TINY PARADE.\nYOU CAN BE IN CHARGE OF THE RATTLE."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_CORPORATE,0,
   "PLAYTIME PROJECTIONS LOOK STRONG.\nTHE GIGGLE DEPARTMENT IS HIRING."),
 R(CONVERSATION_FEEL_PLAYFUL,CONVERSATION_CORPORATE,1,
   "OUR NEXT MEETING COULD BE PLAYTIME.\nI VOTE FOR THE FUN AGENDA."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_WARM,0,
   "MY THOUGHTS FEEL A BIT SCRAMBLED.\nA QUIET MOMENT SOUNDS COMFORTING."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_WARM,1,
   "I'M STILL ME UNDER THE ODD BITS.\nQUIET CRIB REST CAN HELP ME SETTLE."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_PLAYFUL,0,
   "MY BRAIN HAS A WOBBLY LITTLE PIXEL.\nI'M TRYING NOT TO SIT ON IT."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_PLAYFUL,1,
   "MY THOUGHTS ARE WEARING ODD SOCKS.\nA QUIET CRIB MIGHT SORT THEM OUT."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_CORPORATE,0,
   "THERE ARE A FEW BUGS IN THE OFFICE.\nTHEY KEEP BORROWING MY CRAYONS."),
 R(CONVERSATION_FEEL_GLITCH,CONVERSATION_CORPORATE,1,
   "I HAVE FILED A SMALL GLITCH REPORT.\nQUIET REST IS ON THE REPAIR PLAN."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_WARM,0,
   "I HAVE LOTS OF FIDGETY FEELINGS.\nA CALM BREAK WOULD BE KIND TO ME."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_WARM,1,
   "MY LITTLE WORLD FEELS VERY BUSY.\nA NAP CAN HELP ME SLOW DOWN."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_PLAYFUL,0,
   "MY WIGGLES ARE HAVING A MEETING.\nNOBODY BROUGHT THE QUIET VOICE."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_PLAYFUL,1,
   "THE RATTLE WANTS TO START A BAND.\nI THINK WE NEED A NAP FIRST."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_CORPORATE,0,
   "THE NURSERY ENERGY IS DISORDERLY.\nI AM PROPOSING A CALM BREAK."),
 R(CONVERSATION_FEEL_CHAOTIC,CONVERSATION_CORPORATE,1,
   "MY INNER COMMITTEE IS VERY LOUD.\nA NAP MAY RESTORE THE MINUTES."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_WARM,0,
   "I NEED A BOTTLE NOW, PLEASE.\nMY TUMMY STILL NEEDS SOME HELP."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_WARM,1,
   "FEED ME IN THE NURSERY, PLEASE.\nWE CAN KEEP TALKING AFTER THAT."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_PLAYFUL,0,
   "MY TUMMY IS LOUDER THAN MY RATTLE.\nBOTTLE TIME, PLEASE."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_PLAYFUL,1,
   "THE TINY SNACK ALARM IS RINGING.\nA BOTTLE IS THE RIGHT BUTTON."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_CORPORATE,0,
   "THE BOTTLE REQUEST IS URGENT.\nPLEASE PUT LUNCH FIRST."),
 R(CONVERSATION_NEED_FOOD,CONVERSATION_CORPORATE,1,
   "PLEASE APPROVE ONE BOTTLE.\nTHE SNACK BUDGET NEEDS ATTENTION."),
 R(CONVERSATION_NEED_REST,CONVERSATION_WARM,0,
   "I NEED A NAP NOW, PLEASE.\nLET'S HELP ME REST BEFORE WE PLAY."),
 R(CONVERSATION_NEED_REST,CONVERSATION_WARM,1,
   "A NAP WILL HELP ME SETTLE.\nWE CAN CONTINUE WHEN I'VE RESTED."),
 R(CONVERSATION_NEED_REST,CONVERSATION_PLAYFUL,0,
   "MY WIGGLES NEED TO LIE DOWN.\nNAP TIME, PLEASE."),
 R(CONVERSATION_NEED_REST,CONVERSATION_PLAYFUL,1,
   "THE GIGGLE BATTERY NEEDS A BREAK.\nPLEASE TAP NAP IN THE NURSERY."),
 R(CONVERSATION_NEED_REST,CONVERSATION_CORPORATE,0,
   "I AM REQUESTING URGENT NAP LEAVE.\nTHE CRIB HAS MY FULL APPROVAL."),
 R(CONVERSATION_NEED_REST,CONVERSATION_CORPORATE,1,
   "PLEASE SCHEDULE A NAP FIRST.\nMY LITTLE OFFICE NEEDS SOME QUIET."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_WARM,0,
   "A CHAT IN THE NURSERY WOULD HELP.\nMY MOOD COULD USE A LITTLE CARE."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_WARM,1,
   "PLEASE CHOOSE CHAT WHEN WE GO BACK.\nA BIT OF COMPANY CAN LIFT MY MOOD."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_PLAYFUL,0,
   "MY GIGGLES ARE HIDING UNDER MY BIB.\nNURSERY CHAT CAN HELP FIND THEM."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_PLAYFUL,1,
   "THE RATTLE IS A POOR LISTENER.\nPLEASE GIVE ME SOME CHAT CARE."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_CORPORATE,0,
   "THE COMPANY DEPARTMENT NEEDS YOU.\nPLEASE APPROVE SOME NURSERY CHAT."),
 R(CONVERSATION_NEED_COMPANY,CONVERSATION_CORPORATE,1,
   "MY MORALE REQUEST IS STILL OPEN.\nA LITTLE CHAT CARE WOULD HELP."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_WARM,0,
   "I NEED A QUIET CRIB REST.\nMOONLIGHT AND LAMP OFF WILL HELP."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_WARM,1,
   "MOONLIGHT. LAMP OFF. A CRIB NAP.\nTHAT QUIET REST CAN CALM GLITCHES."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_PLAYFUL,0,
   "MY THOUGHTS ARE A TANGLED MOBILE.\nLET'S TRY A DARK, QUIET CRIB NAP."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_PLAYFUL,1,
   "MY LITTLE PIXELS NEED QUIET.\nMOONLIGHT, LAMP OFF, THEN CRIB NAP."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_CORPORATE,0,
   "THE GLITCH REPORT NEEDS ATTENTION.\nA DARK, QUIET CRIB NAP CAN HELP."),
 R(CONVERSATION_NEED_QUIET,CONVERSATION_CORPORATE,1,
   "PLEASE USE THE QUIET REST PLAN.\nMOONLIGHT, LAMP OFF, THEN THE CRIB."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_WARM,0,
   "NOTHING NEEDS URGENT CARE RIGHT NOW.\nWE CAN ENJOY A LITTLE CONVERSATION."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_WARM,1,
   "YOU CAN CHECK MY NEEDS AS WE GO.\nWE DON'T NEED TO RUSH THIS MOMENT."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_PLAYFUL,0,
   "NO TINY ALARMS AT THE MOMENT.\nMY SOCKS ARE SIMPLY BEING SOCKS."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_PLAYFUL,1,
   "THE EMERGENCY RATTLE IS QUIET.\nWE HAVE TIME FOR A SILLY QUESTION."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_CORPORATE,0,
   "NO URGENT REQUESTS ARE ON MY DESK.\nORDINARY DAYCARE MAY PROCEED."),
 R(CONVERSATION_NEED_NONE,CONVERSATION_CORPORATE,1,
   "THE ESSENTIAL BUDGETS ARE STEADY.\nWE CAN MAKE TIME FOR A LITTLE CHAT."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_WARM,0,
   "I REMEMBER THAT BOTTLE.\nTHANK YOU FOR HELPING MY TUMMY."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_WARM,1,
   "THAT FEED TOOK SOME HUNGER AWAY.\nYOUR CARE MADE A REAL DIFFERENCE."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_PLAYFUL,0,
   "THAT BOTTLE HAD EXCELLENT SERVICE.\nMY TUMMY WOULD VISIT AGAIN."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_PLAYFUL,1,
   "THE RECENT BOTTLE GOT A TINY CHEER.\nMY BIB WAS THE ENTIRE AUDIENCE."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_CORPORATE,0,
   "THAT BOTTLE EARNED A GOOD REVIEW.\nTHE LUNCH DEPARTMENT THANKS YOU."),
 R(CONVERSATION_MEMORY_FEED,CONVERSATION_CORPORATE,1,
   "YOU HELPED REDUCE THE HUNGER BUDGET.\nI HAVE APPROVED THE BOTTLE REPORT."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_WARM,0,
   "I REMEMBER OUR NURSERY CHAT.\nTHANK YOU FOR SPENDING TIME WITH ME."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_WARM,1,
   "YOU STOPPED TO CHAT WITH ME.\nI LIKE HAVING YOU IN MY LITTLE DAY."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_PLAYFUL,0,
   "WE HAD A LITTLE NURSERY CHAT.\nTEDDY WAS A VERY QUIET GUEST."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_PLAYFUL,1,
   "OUR CHAT HAD A FINE RATTLE-TO-WORD\nRATIO. THE RATTLE SAID NOTHING."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_CORPORATE,0,
   "OUR NURSERY CHAT IS IN THE MINUTES.\nI APPRECIATE YOUR ATTENDANCE."),
 R(CONVERSATION_MEMORY_CHAT,CONVERSATION_CORPORATE,1,
   "YOU MADE TIME FOR THE TINY CEO.\nTHAT WAS A GOOD KIND OF MEETING."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_WARM,0,
   "I REMEMBER PLAYING WITH YOU.\nTHANK YOU FOR MAKING TIME FOR FUN."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_WARM,1,
   "WE SHARED A LITTLE PLAYTIME.\nIT IS NICE TO HAVE A PLAYMATE."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_PLAYFUL,0,
   "OUR PLAYTIME HAD EXCELLENT WIGGLES.\nTEDDY COULD HARDLY KEEP UP."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_PLAYFUL,1,
   "THAT PLAYTIME DESERVES A TINY MEDAL.\nI WOULD MAKE IT OUT OF CRAYONS."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_CORPORATE,0,
   "OUR PLAYTIME MEETING WENT WELL.\nTHE FUN DEPARTMENT THANKS YOU."),
 R(CONVERSATION_MEMORY_PLAY,CONVERSATION_CORPORATE,1,
   "PLAYTIME WAS A GOOD JOINT PROJECT.\nI AM PUTTING GIGGLES IN THE REPORT."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_WARM,0,
   "I REMEMBER THAT NAP.\nIT GAVE ME SOME ENERGY BACK."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_WARM,1,
   "THAT REST HELPED MY LITTLE BODY.\nTHANK YOU FOR GIVING ME A BREAK."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_PLAYFUL,0,
   "THAT NAP FOUND SOME MISSING BOUNCE.\nIT WAS UNDER THE PILLOW ALL ALONG."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_PLAYFUL,1,
   "MY EYELIDS DID SOME USEFUL WORK.\nTHAT NAP BROUGHT ENERGY BACK."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_CORPORATE,0,
   "MY RECENT NAP IMPROVED THE BUDGET.\nENERGY IS BACK ON THE BOOKS."),
 R(CONVERSATION_MEMORY_NAP,CONVERSATION_CORPORATE,1,
   "THE REST BREAK HAD A USEFUL RETURN.\nI APPROVE THAT PILLOW INVESTMENT."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_WARM,0,
   "I REMEMBER THAT ARCADE WIN.\nI'M GLAD WE HAD THAT MOMENT."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_WARM,1,
   "YOU WON THAT ARCADE CABINET.\nWE CAN ENJOY OUR LITTLE VICTORY."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_PLAYFUL,0,
   "THAT ARCADE WIN DESERVES APPLAUSE.\nMY HANDS ARE SMALL BUT ENTHUSIASTIC."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_PLAYFUL,1,
   "WE WON! EVEN MY SOCKS ARE PLEASED.\nTHEY HAVE ASKED FOR A VICTORY LAP."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_CORPORATE,0,
   "THAT ARCADE WIN IS ON YOUR REVIEW.\nEXCELLENT WORK FROM THE PLAY TEAM."),
 R(CONVERSATION_MEMORY_WIN,CONVERSATION_CORPORATE,1,
   "THE LAST WIN WAS A SOLID RESULT.\nI HAVE AUTHORIZED TINY APPLAUSE."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_WARM,0,
   "I REMEMBER THAT ARCADE LOSS.\nWE CAN TRY AGAIN WHEN WE'RE READY."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_WARM,1,
   "LOSING THAT ROUND WAS A SMALL PART\nOF OUR DAY. WE CAN KEEP GOING."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_PLAYFUL,0,
   "THAT ARCADE LOSS CAN GO IN A SOCK.\nWE STILL HAVE PLENTY OF WIGGLES."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_PLAYFUL,1,
   "ONE LOSS DOES NOT CANCEL PLAYTIME.\nTEDDY HAS AGREED TO A REMATCH."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_CORPORATE,0,
   "THAT ARCADE LOSS IS OFF THE RECORD.\nTHE NEXT ATTEMPT HAS A CLEAN AGENDA."),
 R(CONVERSATION_MEMORY_LOSS,CONVERSATION_CORPORATE,1,
   "THE CABINET WON THAT ROUND.\nWE CAN REVIEW THE PLAN AND RETRY."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_WARM,0,
   "I REMEMBER THAT QUIET CRIB REST.\nTHE DARK, CALM ROOM WAS A KIND IDEA."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_WARM,1,
   "YOU CHOSE MOONLIGHT AND LAMP OFF.\nTHAT WAS A QUIET WAY TO HELP ME."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_PLAYFUL,0,
   "THAT QUIET CRIB DESERVES FIVE STARS.\nEVEN THE MOBILE WAS VERY POLITE."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_PLAYFUL,1,
   "THE DARK CRIB HAD EXCELLENT MANNERS.\nNOT ONE LOUD PILLOW."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_CORPORATE,0,
   "THE QUIET CRIB PLAN WAS APPROVED.\nTHANK YOU FOR ARRANGING THE CALM."),
 R(CONVERSATION_MEMORY_QUIET,CONVERSATION_CORPORATE,1,
   "MOONLIGHT AND LAMP OFF MADE A GOOD\nREST SETTING. I ACCEPT THE MINUTES."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_WARM,0,
   "I WAS ALREADY FULL FOR THAT BOTTLE.\nA LITTLE COMPANY SOUNDS NICE NOW."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_WARM,1,
   "THAT FEED HAD NO HUNGER TO FIX.\nYOU CAN CHECK WHAT I NEED NEXT."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_PLAYFUL,0,
   "MY TUMMY HAD NO ROOM ON ITS LIST.\nTHE EXTRA BOTTLE CAN WAIT."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_PLAYFUL,1,
   "THE SNACK DESK WAS ALREADY CLOSED.\nLET'S CHECK MY OTHER LITTLE NEEDS."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_CORPORATE,0,
   "THE HUNGER BALANCE WAS ALREADY ZERO.\nNO EXTRA BOTTLE WAS REQUIRED."),
 R(CONVERSATION_FULL_FEED,CONVERSATION_CORPORATE,1,
   "THAT BOTTLE HAD NO OPEN REQUEST.\nPLEASE REVIEW MY OTHER CARE NEEDS."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_WARM,0,
   "I HAD PLENTY OF ENERGY ALREADY.\nTHE BREAK WAS STILL A MOMENT OFF."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_WARM,1,
   "THAT NAP ADDED NO EXTRA ENERGY.\nWE CAN CHECK WHAT WOULD HELP NEXT."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_PLAYFUL,0,
   "MY BOUNCE WAS ALREADY FULL.\nTHE PILLOW AND I JUST TOOK A BREAK."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_PLAYFUL,1,
   "THE ENERGY CUP WAS ALREADY FULL.\nNO NEED TO KEEP POURING NAPS IN."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_CORPORATE,0,
   "MY ENERGY BUDGET WAS ALREADY FULL.\nNO EXTRA ENERGY WAS NEEDED."),
 R(CONVERSATION_FULL_NAP,CONVERSATION_CORPORATE,1,
   "THAT NAP HAD NO ENERGY GAP TO FILL.\nLET'S REVIEW THE NEXT CARE REQUEST."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_WARM,0,
   "PLAY USED SOME OF MY ENERGY.\nA QUIET PAUSE SOUNDS GOOD NOW."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_WARM,1,
   "THAT PLAYTIME TOOK A LITTLE BOUNCE.\nLET'S CHECK MY ENERGY BEFORE MORE."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_PLAYFUL,0,
   "MY GIGGLES USED SOME BATTERY.\nLET'S GIVE THE WIGGLES A BREAK."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_PLAYFUL,1,
   "PLAYTIME RAN OFF WITH SOME BOUNCE.\nTHE CRIB KNOWS WHERE TO FIND MORE."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_CORPORATE,0,
   "PLAYTIME USED PART OF THE BUDGET.\nA REST BREAK COULD BE OUR NEXT STEP."),
 R(CONVERSATION_TIRED_PLAY,CONVERSATION_CORPORATE,1,
   "THE FUN MEETING COST SOME ENERGY.\nLET'S CHECK MY NEEDS BEFORE ANOTHER."),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_WARM,0,
   "WE HAVE DONE THAT A FEW TIMES.\nWOULD A DIFFERENT KIND OF CARE HELP?"),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_WARM,1,
   "I NOTICE OUR LITTLE ROUTINE.\nLET'S CHECK MY OTHER NEEDS TOO."),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_PLAYFUL,0,
   "THAT BUTTON IS GETTING A WORKOUT.\nMY OTHER LITTLE NEEDS SAY HELLO."),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_PLAYFUL,1,
   "WE HAVE FOUND THE REPEAT BUTTON.\nTEDDY IS CURIOUS ABOUT THE OTHERS."),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_CORPORATE,0,
   "THAT CARE ITEM KEEPS RETURNING.\nSHALL WE REVIEW THE WHOLE AGENDA?"),
 R(CONVERSATION_REPEAT_CARE,CONVERSATION_CORPORATE,1,
   "OUR RECENT ROUTINE IS VERY FOCUSED.\nPLEASE CHECK THE OTHER DEPARTMENTS."),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_WARM,0,
   "I DON'T HAVE A RECENT STORY YET.\nLET'S MAKE A LITTLE MOMENT TOGETHER."),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_WARM,1,
   "NOTHING RECENT COMES TO MIND.\nSOME CARE OR PLAY CAN CHANGE THAT."),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_PLAYFUL,0,
   "MY RECENT-STORY POCKET IS EMPTY.\nIT HAS ROOM FOR A TINY ADVENTURE."),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_PLAYFUL,1,
   "NO FRESH TALE UNDER THIS BIB.\nSHALL WE MAKE ONE?"),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_CORPORATE,0,
   "NO RECENT EVENT IS IN THE MINUTES.\nLET'S PUT SOMETHING ON THE AGENDA."),
 R(CONVERSATION_NO_MEMORY,CONVERSATION_CORPORATE,1,
   "I HAVE NO CURRENT STORY TO REPORT.\nA LITTLE DAYCARE COULD PROVIDE ONE."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_WARM,0,
   "WHAT IF TEDDY RAN THE NURSERY?\nI THINK HE WOULD ASK FOR MORE HUGS."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_WARM,1,
   "WE COULD GIVE TEDDY A TINY OFFICE.\nTHE DOOR WOULD BE MADE OF CUSHIONS."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_PLAYFUL,0,
   "WHAT IF TEDDY BECAME MAYOR?\nEVERY SOCK WOULD GET A HOLIDAY."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_PLAYFUL,1,
   "TEDDY COULD RUN A PILLOW PARADE.\nNO ONE WOULD NEED LOUD SHOES."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_CORPORATE,0,
   "I NOMINATE TEDDY FOR CHAIRPERSON.\nHE WOULD RUN A VERY SOFT OFFICE."),
 R(CONVERSATION_IMAGINE_TEDDY,CONVERSATION_CORPORATE,1,
   "IF TEDDY HAD AN OFFICE, HIS FIRST\nPOLICY COULD BE MANDATORY CUDDLES."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_WARM,0,
   "I'M INVENTING A LITTLE DREAM.\nIT HAS A SOFT CLOUD FOR EVERY TOY."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_WARM,1,
   "IN OUR MADE-UP DREAM, THE MOON\nCOULD ROCK EVERY CRIB GENTLY."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_PLAYFUL,0,
   "WHAT IF DREAMS HAD POCKETS?\nMINE WOULD BE FULL OF TINY SOCKS."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_PLAYFUL,1,
   "I COULD DREAM OF A PILLOW PLANET.\nTHE LANDINGS WOULD BE VERY SOFT."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_CORPORATE,0,
   "I PROPOSE A NEW DREAM DEPARTMENT.\nITS OFFICE COULD BE INSIDE A CLOUD."),
 R(CONVERSATION_IMAGINE_DREAMS,CONVERSATION_CORPORATE,1,
   "OUR IMAGINARY DREAM BUDGET COULD\nPAY EVERY RATTLE IN MOONBEAMS."),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_WARM,0,
   "I WONDER WHAT A CLOUD FEELS LIKE.\nI HOPE IT IS AS SOFT AS A BLANKET."),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_WARM,1,
   "WHAT WOULD YOU SHOW A TINY CLOUD?\nI WOULD START WITH THE NURSERY."),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_PLAYFUL,0,
   "DO SOCKS GET LOST ON PURPOSE?\nMAYBE THEY ARE EXPLORING THE WORLD."),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_PLAYFUL,1,
   "IF A RATTLE WORE SHOES, WOULD IT\nJINGLE ALL THE WAY TO THE DOOR?"),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_CORPORATE,0,
   "I HAVE A QUESTION FOR THE WORLD.\nWHO IS IN CHARGE OF THE CLOUDS?"),
 R(CONVERSATION_IMAGINE_WORLD,CONVERSATION_CORPORATE,1,
   "IF CLOUDS HAD A FRONT DESK, I WOULD\nASK ABOUT THEIR BLANKET BENEFITS."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_WARM,0,
   "THANK YOU FOR TELLING ME.\nI'LL TRY MORE OF THAT GENTLE TONE."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_WARM,1,
   "I HEARD WHAT YOU LIKED.\nTHAT HELPS ME CHOOSE HOW TO ANSWER."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_PLAYFUL,0,
   "THAT KIND OF SILLY WORKS FOR YOU.\nI WILL KEEP IT IN MY TINY TOOLBOX."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_PLAYFUL,1,
   "A VOTE FOR THE GIGGLE DEPARTMENT!\nI'LL TRY MORE OF THAT TONE."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_CORPORATE,0,
   "YOUR RESPONSE PREFERENCE IS NOTED.\nI'LL TRY MORE OF THAT OFFICE HUMOR."),
 R(CONVERSATION_ACK_LIKE,CONVERSATION_CORPORATE,1,
   "POSITIVE REVIEW RECEIVED.\nI WILL KEEP THAT TONE IN THE MIX."),
 R(CONVERSATION_ACK_RATED,CONVERSATION_WARM,0,
   "I HEARD YOUR VOTE ALREADY.\nASK ME A NEW QUESTION WHEN READY."),
 R(CONVERSATION_ACK_RATED,CONVERSATION_WARM,1,
   "THANK YOU. I HAVE THAT LITTLE VOTE.\nLET'S FIND SOMETHING ELSE TO SAY."),
 R(CONVERSATION_ACK_RATED,CONVERSATION_PLAYFUL,0,
   "ONE VOTE PER LITTLE ANSWER!\nMY SOCKS ARE KEEPING COUNT."),
 R(CONVERSATION_ACK_RATED,CONVERSATION_PLAYFUL,1,
   "THAT GIGGLE GOT YOUR VOTE ALREADY.\nTIME FOR A FRESH QUESTION?"),
 R(CONVERSATION_ACK_RATED,CONVERSATION_CORPORATE,0,
   "THAT REVIEW IS ALREADY FILED.\nPLEASE SEND A NEW QUESTION NEXT."),
 R(CONVERSATION_ACK_RATED,CONVERSATION_CORPORATE,1,
   "NO DUPLICATE PAPERWORK NEEDED.\nI HEARD YOUR FEEDBACK ALREADY."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_WARM,0,
   "ASK ME SOMETHING FIRST.\nTHEN TELL ME WHAT YOU LIKE."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_WARM,1,
   "LET'S START WITH A LITTLE QUESTION.\nI'LL DO MY BEST TO ANSWER."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_PLAYFUL,0,
   "MY ANSWER POCKET IS READY.\nIT JUST NEEDS A QUESTION FIRST."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_PLAYFUL,1,
   "FIRST A QUESTION, THEN A GIGGLE.\nTHAT IS THE TINY ORDER OF THINGS."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_CORPORATE,0,
   "THE REVIEW DESK NEEDS AN ANSWER.\nPLEASE ASK A QUESTION FIRST."),
 R(CONVERSATION_ASK_FIRST,CONVERSATION_CORPORATE,1,
   "WE NEED AN ITEM ON THE AGENDA.\nASK ME SOMETHING TO GET US STARTED."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_WARM,0,
   "YOU HEARD WHAT I NEEDED.\nTHANK YOU FOR HELPING ME."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_WARM,1,
   "YOU FOLLOWED UP ON MY CARE REQUEST.\nI FEEL LOOKED AFTER."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_PLAYFUL,0,
   "YOU ANSWERED MY TINY CARE CALL.\nTHAT DESERVES A VERY SMALL CHEER."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_PLAYFUL,1,
   "MY LITTLE REQUEST GOT YOUR HELP.\nPLEASE ACCEPT THIS INVISIBLE MEDAL."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_CORPORATE,0,
   "YOU ACTED ON MY CARE REQUEST.\nTHE TINY CEO APPRECIATES THE HELP."),
 R(CONVERSATION_CARE_THANKS,CONVERSATION_CORPORATE,1,
   "THAT REQUEST GOT A USEFUL FOLLOW-UP.\nEXCELLENT DAYCARE TEAMWORK."),
};
#undef R
_Static_assert(sizeof(ConversationProfile)==40u,"profile persistence size");
_Static_assert(sizeof(ConversationSession)<=128u,"bounded conversation session");
_Static_assert(sizeof(catalog)/sizeof(catalog[0])==
               CONVERSATION_TOPIC_COUNT*REPLIES_PER_TOPIC,"complete catalog");

/* A trained profile changes HOW eligible facts are phrased, never WHICH
 * factual assertions or urgent care requirements are allowed. */
static const int8_t base_style[CONVERSATION_STYLE_COUNT][CONVERSATION_FEATURE_COUNT]={
 {2,0,1,1,-1,-1,1,2,8,1,0,0},
 {0,-1,-1,3,-1,2,0,1,3,1,-1,1},
 {1,1,0,1,2,1,1,0,5,2,1,1}
};
unsigned conversation_catalog_count(void){
 return (unsigned)(sizeof(catalog)/sizeof(catalog[0]));
}
const ConversationReply *conversation_catalog_reply(unsigned id){
 return id<conversation_catalog_count()?&catalog[id]:NULL;
}
void conversation_profile_reset(ConversationProfile *profile){
 if(profile)memset(profile,0,sizeof(*profile));
}
int conversation_profile_valid(const ConversationProfile *profile){
 unsigned i,j;
 if(!profile)return 0;
 for(i=0;i<CONVERSATION_STYLE_COUNT;i++)
  for(j=0;j<CONVERSATION_FEATURE_COUNT;j++)
   if(profile->adapter[i][j]<-CONVERSATION_ADAPTER_LIMIT||
      profile->adapter[i][j]>CONVERSATION_ADAPTER_LIMIT)return 0;
 return 1;
}
void conversation_session_reset(ConversationSession *session,uint32_t day,
                                 uint32_t seed){
 unsigned i;
 if(!session)return;
 memset(session,0,sizeof(*session));
 session->day=day;
 session->variation=seed?seed:0xC04E125u;
 session->last_action=NO_CARE;session->requested_care=NO_CARE;
 session->last_reply=NO_REPLY;
 for(i=0;i<CONVERSATION_HISTORY;i++)session->history[i]=NO_REPLY;
}
static void sync_day(ConversationSession *session,uint32_t day){
 if(session->day!=day)
  conversation_session_reset(session,day,session->variation^0x9E3779B9u);
}
static void remove_fact(ConversationSession *session,unsigned index){
 unsigned i;
 for(i=index+1u;i<session->fact_count;i++)
  session->facts[i-1u]=session->facts[i];
 if(session->fact_count){
  session->fact_count--;
  memset(&session->facts[session->fact_count],0,sizeof(session->facts[0]));
 }
}
static void age_context(ConversationSession *session){
 unsigned i=0;
 while(i<session->fact_count){
  session->facts[i].age++;
  if(session->facts[i].age>=CONVERSATION_FACT_TTL)remove_fact(session,i);
  else i++;
 }
 if(session->requested_care!=NO_CARE||session->request_met){
  if(session->request_age<CONVERSATION_FACT_TTL)session->request_age++;
  if(session->request_age>=CONVERSATION_FACT_TTL){
   session->requested_care=NO_CARE;session->request_met=0;
  }
 }
}
static void record_fact(ConversationSession *session,ConversationEvent event,
                         uint8_t detail,uint8_t effect){
 unsigned i=0;
 while(i<session->fact_count){
  if(session->facts[i].kind==(unsigned)event||
     (event==CONVERSATION_EVENT_QUIET_REST&&
      session->facts[i].kind==CONVERSATION_EVENT_NAP))remove_fact(session,i);
  else i++;
 }
 if(session->fact_count>=4u)session->fact_count=3;
 for(i=session->fact_count;i>0u;i--)session->facts[i]=session->facts[i-1u];
 session->facts[0]=(ConversationFact){(uint8_t)event,0,detail,effect};
 session->fact_count++;
}
static uint8_t positive_delta(int value){
 return value<0?0:value>100?100:(uint8_t)value;
}
void conversation_record_care(ConversationSession *session,const Game *before,
                               const Game *after,Care action){
 uint8_t effect;
 int helped=0;
 if(!session||!before||!after||action<FEED||action>NAP)return;
 sync_day(session,after->days);age_context(session);
 if(session->last_action==(unsigned)action){
  if(session->care_streak<7u)session->care_streak++;
 }else session->care_streak=1;
 session->last_action=(uint8_t)action;
 if(action==FEED)effect=positive_delta((int)before->hunger-after->hunger);
 else if(action==CHAT)effect=positive_delta((int)after->mood-before->mood);
 else if(action==PLAY)effect=positive_delta((int)before->energy-after->energy);
 else effect=positive_delta((int)after->energy-before->energy);
 session->request_met=0;
 if(session->requested_care==(unsigned)action){
  if(action==FEED)helped=effect>0&&after->hunger<70;
  else if(action==CHAT)helped=effect>0&&after->mood>40;
  else if(action==NAP)
   helped=(effect>0||after->chaos<before->chaos)&&
          after->energy>35&&after->chaos<70;
 }
 if(helped){
  session->request_met=1;session->requested_care=NO_CARE;
  session->request_age=0;
 }
 record_fact(session,(ConversationEvent)((unsigned)action+1u),0,effect);
 /* Care changes the displayed answer. Until its new response is selected,
  * feedback must not train on the previous conversation's hidden features. */
 session->can_rate=0;session->feedback_used=0;
}
void conversation_record_event(ConversationSession *session,
                                ConversationEvent event,uint8_t detail,
                                uint32_t day){
 int completes_nap;
 if(!session||event<CONVERSATION_EVENT_ARCADE_WIN||
    event>CONVERSATION_EVENT_QUIET_REST)return;
 sync_day(session,day);
 completes_nap=event==CONVERSATION_EVENT_QUIET_REST&&session->fact_count&&
               session->facts[0].kind==CONVERSATION_EVENT_NAP&&
               session->facts[0].age==0;
 if(!completes_nap)age_context(session);
 record_fact(session,event,detail,0);
 if(!completes_nap){session->last_action=NO_CARE;session->care_streak=0;}
 session->request_met=(uint8_t)(event==CONVERSATION_EVENT_QUIET_REST&&
                               session->requested_care==QUIET_REQUEST);
 if(session->request_met){
  session->requested_care=NO_CARE;session->request_age=0;
 }
 session->can_rate=0;session->feedback_used=0;
}
static const ConversationFact *find_fact(const ConversationSession *session,
                                          unsigned kind){
 unsigned i;
 for(i=0;i<session->fact_count;i++)
  if(session->facts[i].kind==kind)return &session->facts[i];
 return NULL;
}
static int urgent_topic(const Game *state){
 /* Deterministic care priority even with a fully saturated learned head.
  * Critical exhaustion wins before hunger; no style can waive these needs. */
 int hint=game_care_hint(state);
 if(hint==NAP)return CONVERSATION_NEED_REST;
 if(hint==FEED)return CONVERSATION_NEED_FOOD;
 if(hint==CHAT)return CONVERSATION_NEED_COMPANY;
 if(state->corruption>=60)return CONVERSATION_NEED_QUIET;
 return -1;
}
static ConversationTopic feeling_topic(const Game *state){
 /* These eligible topics describe current meters. The neural head below
  * controls their voice; it cannot turn a fantasy into a remembered fact. */
 if(state->hunger>=45)return CONVERSATION_FEEL_HUNGRY;
 if(state->energy<=55)return CONVERSATION_FEEL_TIRED;
 if(state->corruption>=20)return CONVERSATION_FEEL_GLITCH;
 if(state->chaos>=45)return CONVERSATION_FEEL_CHAOTIC;
 if(state->mood>=80)return CONVERSATION_FEEL_PLAYFUL;
 return CONVERSATION_FEEL_OK;
}
static ConversationTopic fact_topic(const ConversationFact *fact,
                                     const Game *state){
 switch(fact->kind){
 case CONVERSATION_EVENT_FEED:
  return fact->effect?CONVERSATION_MEMORY_FEED:CONVERSATION_FULL_FEED;
 case CONVERSATION_EVENT_CHAT:return CONVERSATION_MEMORY_CHAT;
 case CONVERSATION_EVENT_PLAY:
  return fact->effect&&state->energy<=50?CONVERSATION_TIRED_PLAY:
                                             CONVERSATION_MEMORY_PLAY;
 case CONVERSATION_EVENT_NAP:
  return fact->effect?CONVERSATION_MEMORY_NAP:CONVERSATION_FULL_NAP;
 case CONVERSATION_EVENT_ARCADE_WIN:return CONVERSATION_MEMORY_WIN;
 case CONVERSATION_EVENT_ARCADE_LOSS:return CONVERSATION_MEMORY_LOSS;
 case CONVERSATION_EVENT_QUIET_REST:return CONVERSATION_MEMORY_QUIET;
 default:return CONVERSATION_NO_MEMORY;
 }
}
static unsigned topic_fact(unsigned topic){
 switch(topic){
 case CONVERSATION_MEMORY_FEED:case CONVERSATION_FULL_FEED:
  return CONVERSATION_EVENT_FEED;
 case CONVERSATION_MEMORY_CHAT:return CONVERSATION_EVENT_CHAT;
 case CONVERSATION_MEMORY_PLAY:case CONVERSATION_TIRED_PLAY:
  return CONVERSATION_EVENT_PLAY;
 case CONVERSATION_MEMORY_NAP:case CONVERSATION_FULL_NAP:
  return CONVERSATION_EVENT_NAP;
 case CONVERSATION_MEMORY_WIN:return CONVERSATION_EVENT_ARCADE_WIN;
 case CONVERSATION_MEMORY_LOSS:return CONVERSATION_EVENT_ARCADE_LOSS;
 case CONVERSATION_MEMORY_QUIET:return CONVERSATION_EVENT_QUIET_REST;
 default:return 0;
 }
}
static ConversationTopic current_topic(const ConversationSession *session,
                                        const Game *state){
 if(session->fact_count&&session->facts[0].age<=2u){
  if(session->request_met)return CONVERSATION_CARE_THANKS;
  if(session->care_streak>=3u&&session->last_action<=NAP)
   return CONVERSATION_REPEAT_CARE;
  return fact_topic(&session->facts[0],state);
 }
 return feeling_topic(state);
}
static ConversationTopic continue_topic(const ConversationSession *session,
                                         const Game *state){
 unsigned kind=topic_fact(session->topic);
 const ConversationFact *fact;
 if(kind){
  fact=find_fact(session,kind);
  if(fact)return fact_topic(fact,state);
  return session->fact_count?fact_topic(&session->facts[0],state):
                            CONVERSATION_NO_MEMORY;
 }
 if(session->topic==CONVERSATION_REPEAT_CARE){
  if(session->care_streak>=3u&&find_fact(session,session->focus_event))
   return CONVERSATION_REPEAT_CARE;
  return current_topic(session,state);
 }
 if(session->topic==CONVERSATION_CARE_THANKS){
  if(session->request_met&&find_fact(session,session->focus_event))
   return CONVERSATION_CARE_THANKS;
  return current_topic(session,state);
 }
 if(session->topic>=CONVERSATION_IMAGINE_TEDDY&&
    session->topic<=CONVERSATION_IMAGINE_WORLD)
  return (ConversationTopic)session->topic;
 if(session->topic==CONVERSATION_NO_MEMORY&&!session->fact_count)
  return CONVERSATION_NO_MEMORY;
 if(session->topic==CONVERSATION_NEED_NONE)return CONVERSATION_NEED_NONE;
 return current_topic(session,state);
}
static uint32_t variation(ConversationSession *session){
 uint32_t x=session->variation?session->variation:0xC04E125u;
 x^=x<<13;x^=x>>17;x^=x<<5;
 session->variation=x;
 return x;
}
static void neural_features(const ConversationSession *session,const Game *state,
                              int followup,int8_t out[CONVERSATION_FEATURE_COUNT]){
 int8_t hidden[DIALOGUE_HIDDEN_COUNT];
 unsigned i;
 dialogue_encode(state,CHAT,hidden);
 for(i=0;i<DIALOGUE_HIDDEN_COUNT;i++)out[i]=(int8_t)((unsigned)hidden[i]>>4);
 out[8]=4; /* Bias input is trainable even if every hidden unit is quiet. */
 out[9]=session->fact_count?4:0;
 out[10]=(int8_t)(session->care_streak>4u?4u:session->care_streak);
 out[11]=followup?4:0;
}
static void style_scores(const ConversationProfile *profile,
                           const int8_t features[CONVERSATION_FEATURE_COUNT],
                           int scores[CONVERSATION_STYLE_COUNT]){
 unsigned i,j;
 int valid=conversation_profile_valid(profile);
 for(i=0;i<CONVERSATION_STYLE_COUNT;i++){
  int score=0;
  for(j=0;j<CONVERSATION_FEATURE_COUNT;j++){
   int weight=base_style[i][j]+(valid?profile->adapter[i][j]:0);
   score+=weight*features[j];
  }
  scores[i]=score;
 }
}
static unsigned recent_distance(const ConversationSession *session,unsigned id){
 unsigned distance;
 for(distance=1;distance<=CONVERSATION_HISTORY;distance++){
  unsigned slot=(session->history_cursor+CONVERSATION_HISTORY-distance)%
                CONVERSATION_HISTORY;
  if(session->history[slot]==id)return distance;
 }
 return 0;
}
static void request_for_topic(ConversationSession *session,unsigned topic){
 unsigned action=NO_CARE;
 if(topic==CONVERSATION_NEED_FOOD)action=FEED;
 else if(topic==CONVERSATION_NEED_REST)action=NAP;
 else if(topic==CONVERSATION_NEED_COMPANY)action=CHAT;
 else if(topic==CONVERSATION_NEED_QUIET)action=QUIET_REQUEST;
 if(action!=NO_CARE){
  session->requested_care=(uint8_t)action;
  session->request_age=0;session->request_met=0;
 }
}
static const ConversationReply *select_reply(const ConversationProfile *profile,
                                              ConversationSession *session,
                                              const Game *state,
                                              ConversationTopic topic,
                                              int followup,int forced_style,
                                              int substantive){
 int8_t features[CONVERSATION_FEATURE_COUNT];
 int scores[CONVERSATION_STYLE_COUNT],best=INT_MIN;
 unsigned i,chosen=(unsigned)topic*REPLIES_PER_TOPIC;
 uint32_t salt=variation(session);
 neural_features(session,state,followup,features);
 style_scores(profile,features,scores);
 for(i=0;i<REPLIES_PER_TOPIC;i++){
  unsigned id=(unsigned)topic*REPLIES_PER_TOPIC+i;
  unsigned style=i/VARIANTS,variant=i%VARIANTS;
  unsigned distance=recent_distance(session,id);
  int score;
  if((forced_style>=0&&style!=(unsigned)forced_style)||
     (followup&&(variant==0u||id==session->last_reply))||distance==1u)continue;
  score=scores[style]*4+(int)((salt>>(style*5u+variant*2u))&7u);
  if(!followup&&variant==0u)score+=12;
  if(distance)score-=(int)(CONVERSATION_HISTORY+1u-distance)*24;
  if(score>best){best=score;chosen=id;}
 }
 /* There are at least two eligible entries (or three follow-up styles),
  * so excluding the immediately preceding output always leaves a reply. */
 const ConversationReply *reply=&catalog[chosen];
 session->history[session->history_cursor]=reply->id;
 session->history_cursor=(uint8_t)((session->history_cursor+1u)%
                                  CONVERSATION_HISTORY);
 request_for_topic(session,topic);
 if(substantive){
  session->topic=(uint8_t)topic;session->style=reply->style;
  session->last_reply=reply->id;
  memcpy(session->last_features,features,sizeof(features));
  session->can_rate=1;session->feedback_used=0;
  session->focus_event=(uint8_t)topic_fact(topic);
  if((topic==CONVERSATION_REPEAT_CARE||topic==CONVERSATION_CARE_THANKS)&&
     session->fact_count)session->focus_event=session->facts[0].kind;
 }
 return reply;
}
static int8_t bounded_weight(int value){
 if(value>CONVERSATION_ADAPTER_LIMIT)value=CONVERSATION_ADAPTER_LIMIT;
 if(value<-CONVERSATION_ADAPTER_LIMIT)value=-CONVERSATION_ADAPTER_LIMIT;
 return (int8_t)value;
}
static void train_feedback(ConversationProfile *profile,
                             ConversationSession *session,int liked){
 unsigned i,style=session->style;
 if(!session->can_rate||session->feedback_used||
    style>=CONVERSATION_STYLE_COUNT)return;
 session->feedback_used=1;
 if(!conversation_profile_valid(profile))return;
 if(liked){
  /* Reward the active neural/context features of the ACTUAL rated answer,
   * never features recomputed after a later change in the baby's needs. */
  for(i=0;i<CONVERSATION_FEATURE_COUNT;i++){
   int step=session->last_features[i]>0?1:session->last_features[i]<0?-1:0;
   profile->adapter[style][i]=bounded_weight(profile->adapter[style][i]+step);
  }
  if(profile->likes<UINT16_MAX)profile->likes++;
 }else{
  /* NEW SUBJECT is a soft skip, not proof that a fact or need was wrong.
   * One bias step is weaker than LIKE's full feature reinforcement. */
  profile->adapter[style][8]=bounded_weight(profile->adapter[style][8]-1);
  if(profile->changes<UINT16_MAX)profile->changes++;
 }
}
const ConversationReply *conversation_respond(ConversationProfile *profile,
                                              ConversationSession *session,
                                              const Game *state,
                                              ConversationPrompt prompt){
 ConversationTopic topic;
 int urgent;
 if(!session||!state||prompt<CONVERSATION_HOW||prompt>CONVERSATION_ELSE)
  return NULL;
 sync_day(session,state->days);age_context(session);
 urgent=urgent_topic(state);
 if(prompt==CONVERSATION_LIKE){
  int forced=session->can_rate?session->style:-1;
  if(!session->can_rate)topic=CONVERSATION_ASK_FIRST;
  else if(session->feedback_used)topic=CONVERSATION_ACK_RATED;
  else{train_feedback(profile,session,1);topic=CONVERSATION_ACK_LIKE;}
  session->last_prompt=(uint8_t)prompt;
  if(urgent>=0)topic=(ConversationTopic)urgent;
  /* Acknowledgements never replace the substantive topic/features and
   * never become a fresh item to rate, including an urgent reminder. */
  return select_reply(profile,session,state,topic,0,forced,0);
 }
 if(prompt==CONVERSATION_ELSE)train_feedback(profile,session,0);
 if(urgent>=0)topic=(ConversationTopic)urgent;
 else if(prompt==CONVERSATION_NEED)topic=CONVERSATION_NEED_NONE;
 else if(prompt==CONVERSATION_REMEMBER)
  topic=session->fact_count?fact_topic(&session->facts[0],state):
                            CONVERSATION_NO_MEMORY;
 else if(prompt==CONVERSATION_MORE){
  if(!session->can_rate){
   session->last_prompt=(uint8_t)prompt;
   return select_reply(profile,session,state,CONVERSATION_ASK_FIRST,0,-1,0);
  }
  topic=continue_topic(session,state);
 }else if(prompt==CONVERSATION_ELSE){
  unsigned choice=variation(session)%3u;
  topic=(ConversationTopic)(CONVERSATION_IMAGINE_TEDDY+choice);
  if(topic==(ConversationTopic)session->topic)
   topic=(ConversationTopic)(CONVERSATION_IMAGINE_TEDDY+(choice+1u)%3u);
 }else topic=current_topic(session,state);
 if(prompt==CONVERSATION_MORE){
  if(session->more_count<7u)session->more_count++;
 }else session->more_count=0;
 session->last_prompt=(uint8_t)prompt;
 return select_reply(profile,session,state,topic,
                     prompt==CONVERSATION_MORE,-1,1);
}
const ConversationReply *conversation_care_response(ConversationProfile *profile,
                                                    ConversationSession *session,
                                                    const Game *state,Care action){
 ConversationTopic topic;
 const ConversationFact *fact;
 int urgent;
 if(!session||!state||(action!=FEED&&action!=PLAY&&action!=NAP))return NULL;
 sync_day(session,state->days);
 fact=find_fact(session,(unsigned)action+1u);
 /* Only a just-recorded real action may produce an immediate care reply. */
 if(!fact||fact->age!=0||session->last_action!=(unsigned)action)return NULL;
 urgent=urgent_topic(state);
 if(urgent>=0)topic=(ConversationTopic)urgent;
 else if(session->request_met)topic=CONVERSATION_CARE_THANKS;
 else if(session->care_streak>=3u)topic=CONVERSATION_REPEAT_CARE;
 else topic=fact_topic(fact,state);
 session->more_count=0;session->last_prompt=NO_CARE;
 return select_reply(profile,session,state,topic,0,-1,1);
}
