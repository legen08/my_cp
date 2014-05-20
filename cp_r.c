#include<dirent.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<string.h>
#include<limits.h>

int cp_file_file(char* from, char* to) {
	if (!strcmp(from, to)) {
		return;
	}
	int fd1 = open(from, O_RDONLY);
	if (fd1 == -1) {
		perror("Ошибка открытия на чтение");
		return(1);
	}
	int fd2 = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd2 == -1) {
		perror("Ошибка открытия на запись");
		close(fd1);
		return(2);
	}
	void* buf = malloc(500);
	if (buf == NULL) {
	    perror("Ошибка выделения памяти");
	    close(fd1);
	    close(fd2);
	    return(3);
	}
	int n;
	while((n = read(fd1, buf, 500)) > 0) {
		int flag = write(fd2, buf, n);
		if(flag <= 0) {
			perror("Ошибка записи");
			close(fd1);
			close(fd2);
			free(buf);
			return(4);	
	    }
	}
	if (n < 0) {
		perror("Ошибка чтения");
		close(fd1);
		close(fd2);
		free(buf);
		return(5);
	}
	close(fd1);
	close(fd2);
	free(buf);
}

size_t filename(const char *path) {
	if (path == NULL) return ULLONG_MAX;
	size_t pos = 0, i = 0;
	while (path[i])
		if (path[i++] == '/') pos = i;
	return pos;
}

size_t dirname(const char *path) {
	if (path == NULL) return ULLONG_MAX;
	size_t pos = 0, i = 0;
	while (path[i])
		if (path[i++] == '/' && path[i]) pos = i;
	return pos;
}

void cp_file_dir(char* path_from, char* path_to) {
	//mkdir(path_to);
	size_t path_len = strlen(path_to);
	size_t index_name_from = filename(path_from);
	path_from += index_name_from;
	size_t name_len = strlen(path_from);
	char *new_path = malloc(path_len + 1 + name_len + 1);
	strcpy(new_path, path_to);
	strcat(new_path, "/");
	strcat(new_path, path_from);
	path_from -= index_name_from;
	cp_file_file(path_from, new_path);
	free(new_path);
}

void cp_dir_dir(char* path_from, char* path_to, char rec);

void walk(char* path_from, char* path_to, char rec) {
	DIR *dir = opendir(path_from);
	if (dir == NULL) {
		perror(path_from);
	}
	struct dirent* dp;
	while ((dp = readdir(dir)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
			continue;
		size_t path_len = strlen(path_from);
		size_t name_len = strlen(dp->d_name);
		char *new_path = malloc(path_len + 1 + name_len + 1);
		strcpy(new_path, path_from);
		strcat(new_path, "/");
		strcat(new_path, dp->d_name);
		if (dp->d_type & DT_DIR) 
			cp_dir_dir(new_path, path_to, rec);
		if (dp->d_type & DT_REG) 
			cp_file_dir(new_path, path_to);
		free(new_path);
	}
	closedir(dir);
}

void cp_dir_dir(char* path_from, char* path_to, char rec) {
	if (rec == 0) return;
	if (!strcmp(path_from, path_to)) {
		printf("невозможно скопировать каталог в самого себя\n");
		return;
	}
	struct stat stbuf_from;
	if (stat(path_from, &stbuf_from) == -1) 
		fprintf(stderr, "can't access %s\n", path_from);
	size_t path_len = strlen(path_to);
	size_t index_name_from = dirname(path_from);
	path_from += index_name_from;
	size_t name_len = strlen(path_from);
	char *new_path = malloc(path_len + 1 + name_len + 1);
	strcpy(new_path, path_to);
	strcat(new_path, "/");
	strcat(new_path, path_from);
	mkdir(new_path, stbuf_from.st_mode);
	chmod(new_path, stbuf_from.st_mode);
	path_from -= index_name_from;
	walk(path_from, new_path, rec);
	free(new_path);
}
	
	

void cp_start(char* path_from, char* path_to, char rec) {
	struct stat stbuf_from;
	struct stat stbuf_to;
	if (stat(path_from, &stbuf_from) == -1) 
		fprintf(stderr, "can't access %s\n", path_from);
	if (stat(path_to, &stbuf_to) == -1) {
		int fd = open(path_to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd == -1) { 
			perror("Ошибка открытия на запись");
		    close(fd);
		}
		stat(path_to, &stbuf_to);
		//fprintf(stderr, "can't access %s\n", path_to);
	}
	if ((stbuf_from.st_mode & S_IFREG) && (stbuf_to.st_mode & S_IFREG)) 
		cp_file_file(path_from, path_to);
	if ((stbuf_from.st_mode & S_IFREG) && (stbuf_to.st_mode & S_IFDIR)) 
		cp_file_dir(path_from, path_to);
	if ((stbuf_from.st_mode & S_IFDIR) && (stbuf_to.st_mode & S_IFREG))
		perror("Нельзя копировать каталог в файл");
	if ((stbuf_from.st_mode & S_IFDIR) && (stbuf_to.st_mode & S_IFDIR)) 
		cp_dir_dir(path_from, path_to, rec);
}

int main(int argc, char* argv[]) {
	char rec = 0;
	char ch;
	while ((ch = getopt(argc, argv, "R")) != -1) {
		if (ch == 'R')
			rec = 1;
		else {
			fprintf(stderr, "Некорректныый аргумент\n");
			return 1;
		}
	} 
	argv += optind;
	argc -= optind;
	char **p;
	for (p = argv; *p != argv[argc - 1]; ++p)
		cp_start(*p, argv[argc - 1], rec);
	//for (i = 0; i < argc; i++) {
		//cp_start(argv[i], argv[argc - 1], rec);
	//}
	return 0;
}	
