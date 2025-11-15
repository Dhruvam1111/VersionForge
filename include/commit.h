#ifndef COMMIT_H
#define COMMIT_H

/**
 * @brief Creates a new commit object.
 *
 * This function builds the root tree, finds the parent commit,
 * formats the commit data, and saves the new commit object.
 *
 * @param message The commit message.
 * @return 0 on success, -1 on failure.
 */
int do_commit(const char *message);

#endif // COMMIT_H
