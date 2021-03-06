/*
 * Reads the user data for the given user from disk into the given
 * memory structure. Returns the input user pointer if successful
 * or NULL if not.
 */
struct User *ReadUser(int userId, struct User *user);

/*
 * Writes the user data for the given user to disk from the given
 * memory structure. Returns the input user pointer if successful
 * or NULL if not.
 */
struct User *WriteUser(int userId, struct User *user, int newUser);

/*
 * Returns a pointer to the users data in Severmem if the user is
 * logged in. Returns NULL otherwise. If the unreadTexts pointer
 * is non-null is will also be set to point to the user's
 * UnreadTexts structure. (If a user is returned.)
 */
struct User *GetLoggedInUser(int userId, struct UnreadTexts **unreadTexts);

/*
 * Returns a pointer to a User structure for the given user, or NULL
 * if the user is not found (or some other error). The data is copied
 * from Servermem if the user is currently logged in. Otherwise it's read
 * from disk.
 * The returned pointer is to static memory in this function and is only
 * valid until the next time this function is called.
 */
struct User *GetUserData(int userId);

/*
 * Returns a pointer to a User structure for the given user, or NULL
 * if the user is not found (or some other error). If the user is
 * currently logged in the pointer is to the active data structure
 * in Servermem. In this case needsWrite will be set to FALSE as
 * any update will take effect immediately and written to disk by
 * the node where the user is logged in. If not the pointer is to
 * static memory in this function and needsWrite will be set to TRUE.
 * For any change to be permanent WriteUser() must be called.
 */
struct User *GetUserDataForUpdate(int userId, int *needsWrite);

/*
 * Allocates a new or existing userDataSlot for userId logging in
 * in nodeId and fills in the slot number in nodeInfo->userDataSlot.
 * Returns 1 if an existing slot is used, 2 if a new slot is used
 * and 0 if a slot could not be allocated. (This should normally never
 * happen since the maximum number of slots is equal to the maximum number
 * numer of nodes.)
 */
int AllocateUserDataSlot(int nodeId, int userId);

/*
 * Releases the user data slot used on the given nodeId. If this is the
 * last node using that user data slot it will become available for re-use
 * by some other user.
 */
void ReleaseUserDataSlot(int nodeId);

/*
 * Returns the userDataSlot for the given user of they are logged in or
 * -1 if they are not.
 */
int FindUserDataSlot(int userId);
