#ifndef DR_GUESTS_H
#define DR_GUESTS_H

class DrGuestList;

/// Builds every guest and adds the ones that can run to `list`.
void dr_create_guests(DrGuestList *list);

#endif
