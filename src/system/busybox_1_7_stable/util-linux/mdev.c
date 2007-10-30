/* vi: set sw=4 ts=4: */
/*
 *
 * mdev - Mini udev for busybox
 *
 * Copyright 2005 Rob Landley <rob@landley.net>
 * Copyright 2005 Frank Sorenson <frank@tuxrocks.com>
 *
 * Licensed under GPL version 2, see file LICENSE in this tarball for details.
 */

#include "libbb.h"
#include "xregex.h"

struct globals {
	int root_major, root_minor;
};
#define G (*(struct globals*)&bb_common_bufsiz1)
#define root_major (G.root_major)
#define root_minor (G.root_minor)

#define MAX_SYSFS_DEPTH 3 /* prevent infinite loops in /sys symlinks */

static void mdev_symlink(char *m_target, char *m_link)
{
	if (!m_target || !m_link) return;
	if (symlink(m_target, m_link) && errno != EEXIST)
		bb_perror_msg_and_die("symlink %s", m_link);
}

/* mknod in /dev based on a path like "/sys/block/hda/hda1" */
static void make_device(char *path, int delete)
{
 	const char *device_name;
	char *device_path = NULL;
 	int major, minor, type, len, mk_dir=0;
 	mode_t mode = 0660, mode_path = 0660;
	uid_t uid = 0;
	gid_t gid = 0;
	char *temp = path + strlen(path);
	char *command = NULL;

	/* Try to read major/minor string.  Note that the kernel puts \n after
	 * the data, so we don't need to worry about null terminating the string
	 * because sscanf() will stop at the first nondigit, which \n is.  We
	 * also depend on path having writeable space after it. */

	if (!delete) {
		strcat(path, "/dev");
		len = open_read_close(path, temp + 1, 64);
		*temp++ = 0;
		if (len < 1) return;
	}

	/* Determine device name, type, major and minor */

	device_name = bb_basename(path);
	type = (path[5]=='b') ? S_IFBLK : S_IFCHR;

	/* If we have a config file, look up permissions for this device */

	if (ENABLE_FEATURE_MDEV_CONF) {
		char *line, *tok_tmp, *tok_line=NULL, **tok = NULL;
		FILE *file;
		int i;
  
		file = fopen_or_warn("/etc/mdev.conf", "r");
		if (file < 0)
  			goto end_parse;
  
		while ((line = xmalloc_getline(file)) != NULL) {
			char *regex = NULL;
			regex_t match;
			regmatch_t off[2];
			int result, tok_len = 1;
			char *tok_id[2], *tok_id_tmp;
			char *s;
  
			tok_line = strdupa(line);
			if (tok_line[0] == '#' || strlen(tok_line)==0) continue;
  
			for (i=0; i<strlen(tok_line); i++) {
				if (isspace(tok_line[i]) && !isspace(tok_line[i+1]))
					tok_len++;
			}
			tok = (char **) xrealloc(tok, tok_len * sizeof(char *));
  
			for (i=0; (tok_tmp=strtok(tok_line, " \t")); i++) {
				tok[i] = tok_tmp;
				tok_line = NULL;
			}
  
			if (!strcmp(tok[1], "->")) {
				mdev_symlink(tok[2], tok[0]);
				continue;
			}
  
			/* Regex to match this device */
			regex = tok[0];
			xregcomp(&match, regex, REG_EXTENDED);
			result = regexec(&match, device_name, 2, off, 0);
			regfree(&match);
  
			/* If not this device, skip rest of line */
			if (result || off[0].rm_so || off[0].rm_eo != strlen(device_name)) {
				continue;
			}

			/* use substring for device name */
			if (off[1].rm_so != -1) {
				int sub_len = off[1].rm_eo - off[1].rm_so;
				char *tmp = alloca(sub_len + 1);
				strncpy(tmp, device_name + off[1].rm_so, sub_len);
				tmp[sub_len] = '\0';
				device_name = tmp;
			}
  
			for (i=0; (tok_id_tmp=strtok(tok[1], ":")); i++) {
				if (tok_id_tmp) tok_id[i] = tok_id_tmp;
				tok[1] = NULL;
			}
  
			/* uid:gid */
			uid = strtoul(tok_id[0], &s, 10);
			if (tok_id[0] == s) {
				struct passwd *pass;
				pass = getpwnam(tok_id[0]);
				if (!pass) continue;
				uid = pass->pw_uid;
			}
  
			gid = strtoul(tok_id[1], &s, 10);
			if (tok_id[1] == s) {
				struct group *grp;
				grp = getgrnam(tok_id[1]);
				if (!grp) continue;
				gid = grp->gr_gid;
			}

			/* mode */
			mode = (mode_t)strtoul(tok[2], &s, 8);

			if (tok_len > 3) {
#if ENABLE_FEATURE_MDEV_EXEC
				const char *s2 = "@$*";
				char *cmd_tmp;
				unsigned int cmd = 0;
#endif
				/* mk_dir */
				if (!strcmp(tok[3], ">>")) {
					int path_len;

					mk_dir = 1;
					device_path = strdupa(tok[4]);
					path_len = strlen(device_path);
					if (device_path[path_len-1] != '/')
						strcat(device_path, "/");

					mode_path = (mode_t)strtoul(tok[5], &s, 8);
  				}
#if ENABLE_FEATURE_MDEV_EXEC
				else {
					if ((cmd_tmp = strpbrk(tok[3], s2))!=NULL) {
						int cmd_len = strlen(cmd_tmp) == 1 ? 1 : 0;

						cmd = *cmd_tmp;

						if (cmd_len == 1) {
							command = strdupa(tok[4]);
						} else {
							command = strdupa(strrchr(tok[3], cmd_tmp[0])+1);
						}

						for (i=4+cmd_len; i<tok_len; i++) {
							strcat(strcat(command, " "), tok[i]);
						}
  					}
					
  				}
  
				if (tok_len > 6) {
					if ((cmd_tmp = strpbrk(tok[6], s2))!=NULL) {
						int cmd_len = strlen(cmd_tmp) == 1 ? 1 : 0;
  
						cmd = *cmd_tmp;
  
						if (cmd_len == 1) {
							command = xstrdup(tok[7]);
						} else {
							command = xstrdup(strrchr(tok[6], cmd_tmp[0])+1);
						}
  
						for (i=7+cmd_len; i<tok_len; i++) {
							strcat(strcat(command, " "), tok[i]);
						}
					}
				}
				switch (cmd) {
				case '@':
					if (delete) command = NULL;
					break;
				case '$':
					if (!delete) command = NULL;
					break;
				case '*':
				default :
					break;
				}
#endif
			}
  		}
		fclose(file);
 end_parse:	/* nothing */ ;
	}

	umask(0);
	if (!delete) {
		if (mk_dir) {
			if (mkdir(device_path, mode_path) && errno != EEXIST)
				bb_perror_msg_and_die("mkdir %s", device_path);
			device_name = strcat(device_path, device_name);
		}
		if (sscanf(temp, "%d:%d", &major, &minor) != 2) return;
		if (mknod(device_name, mode | type, makedev(major, minor)) && errno != EEXIST)
			bb_perror_msg_and_die("mknod %s", device_name);

		if (major == root_major && minor == root_minor)
			symlink(device_name, "root");

		if (ENABLE_FEATURE_MDEV_CONF) chown(device_name, uid, gid);
	}

	if (command) {
		/* setenv will leak memory, so use putenv */
		char *s = xasprintf("MDEV=%s", device_name);
		putenv(s);
		if (system(command) == -1)
			bb_perror_msg_and_die("cannot run %s", command);
		s[4] = '\0';
		unsetenv(s);
		free(s);
	}

	if (delete) {
		if (device_path) {
			char *tmp_path;

			tmp_path = strdupa(device_path);
			device_name = strcat(tmp_path, device_name);
		}

		unlink(device_name);

		if (device_path) {
			if (rmdir(device_path) && errno != ENOTEMPTY)
				bb_perror_msg_and_die("rmdir %s", device_path);
		}
	}

}

/* File callback for /sys/ traversal */
static int fileAction(const char *fileName, struct stat *statbuf,
                      void *userData, int depth)
{
	size_t len = strlen(fileName) - 4;
	char *scratch = userData;

	if (strcmp(fileName + len, "/dev"))
		return FALSE;

	strcpy(scratch, fileName);
	scratch[len] = 0;
	make_device(scratch, 0);

	return TRUE;
}

/* Directory callback for /sys/ traversal */
static int dirAction(const char *fileName, struct stat *statbuf,
                      void *userData, int depth)
{
	return (depth >= MAX_SYSFS_DEPTH ? SKIP : TRUE);
}

/* For the full gory details, see linux/Documentation/firmware_class/README
 *
 * Firmware loading works like this:
 * - kernel sets FIRMWARE env var
 * - userspace checks /lib/firmware/$FIRMWARE
 * - userspace waits for /sys/$DEVPATH/loading to appear
 * - userspace writes "1" to /sys/$DEVPATH/loading
 * - userspace copies /lib/firmware/$FIRMWARE into /sys/$DEVPATH/data
 * - userspace writes "0" (worked) or "-1" (failed) to /sys/$DEVPATH/loading
 * - kernel loads firmware into device
 */
static void load_firmware(const char *const firmware, const char *const sysfs_path)
{
	int cnt;
	int firmware_fd, loading_fd, data_fd;

	/* check for $FIRMWARE from kernel */
	/* XXX: dont bother: open(NULL) works same as open("no-such-file")
	 * if (!firmware)
	 *	return;
	 */

	/* check for /lib/firmware/$FIRMWARE */
	xchdir("/lib/firmware");
	firmware_fd = xopen(firmware, O_RDONLY);

	/* in case we goto out ... */
	data_fd = -1;

	/* check for /sys/$DEVPATH/loading ... give 30 seconds to appear */
	xchdir(sysfs_path);
	for (cnt = 0; cnt < 30; ++cnt) {
		loading_fd = open("loading", O_WRONLY);
		if (loading_fd == -1)
			sleep(1);
		else
			break;
	}
	if (loading_fd == -1)
		goto out;

	/* tell kernel we're loading by `echo 1 > /sys/$DEVPATH/loading` */
	if (write(loading_fd, "1", 1) != 1)
		goto out;

	/* load firmware by `cat /lib/firmware/$FIRMWARE > /sys/$DEVPATH/data */
	data_fd = open("data", O_WRONLY);
	if (data_fd == -1)
		goto out;
	cnt = bb_copyfd_eof(firmware_fd, data_fd);

	/* tell kernel result by `echo [0|-1] > /sys/$DEVPATH/loading` */
	if (cnt > 0)
		write(loading_fd, "0", 1);
	else
		write(loading_fd, "-1", 2);

 out:
	if (ENABLE_FEATURE_CLEAN_UP) {
		close(firmware_fd);
		close(loading_fd);
		close(data_fd);
	}
}

int mdev_main(int argc, char *argv[]);
int mdev_main(int argc, char *argv[])
{
	char *action;
	char *env_path;
	RESERVE_CONFIG_BUFFER(temp,PATH_MAX);

	xchdir("/dev");

	/* Scan */

	if (argc == 2 && !strcmp(argv[1],"-s")) {
		struct stat st;

		xstat("/", &st);
		root_major = major(st.st_dev);
		root_minor = minor(st.st_dev);

		recursive_action("/sys/block",
			ACTION_RECURSE | ACTION_FOLLOWLINKS,
			fileAction, dirAction, temp, 0);

		recursive_action("/sys/class",
			ACTION_RECURSE | ACTION_FOLLOWLINKS,
			fileAction, dirAction, temp, 0);

	/* Hotplug */

	} else {
		action = getenv("ACTION");
		env_path = getenv("DEVPATH");
		if (!action || !env_path)
			bb_show_usage();

		sprintf(temp, "/sys%s", env_path);
		if (!strcmp(action, "remove"))
			make_device(temp, 1);
		else if (!strcmp(action, "add")) {
			make_device(temp, 0);

			if (ENABLE_FEATURE_MDEV_LOAD_FIRMWARE)
				load_firmware(getenv("FIRMWARE"), temp);
		}
	}

	if (ENABLE_FEATURE_CLEAN_UP) RELEASE_CONFIG_BUFFER(temp);
	return 0;
}
