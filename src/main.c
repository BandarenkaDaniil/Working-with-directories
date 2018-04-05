#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <libgen.h>

#include "SLList.h"

void print_error              (const char *module_name, const char* additional_info, const char *error_msg);
bool is_dir                   (const char *dir_path);
int  read_dir_to_list         (const char *curr_dir_path, struct SLList *source_list);
int  filecmp                  (const char *path1, const char *path2);
int  log_file_info            (const char *file_path, const char *outputfile_path);
void cmp_files_lists_with_log (struct SLList *first_list, struct SLList *second_list, const char *log_file);

char *module_name;

int main(int argc, char *argv[])
{
  const int VALID_ARGS_COUNT = 4;

  if (argc < VALID_ARGS_COUNT)
  {
    print_error(module_name, NULL, "too few arguments");
    return 1;
  }

  module_name = basename(argv[0]);

  const char *first_dir_name   = argv[1];
  const char *second_dir_name  = argv[2];
  const char *output_file_name = argv[3];

  if (!is_dir(first_dir_name))
  {
    print_error(module_name, first_dir_name, "not a dir");
    return 1;
  }

  if (!is_dir(second_dir_name))
  {
    print_error(module_name, second_dir_name, "not a dir");
    return 1;
  }

  struct SLList *first_list  = (struct SLList*)malloc(sizeof(struct SLList *));
  struct SLList *second_list = (struct SLList*)malloc(sizeof(struct SLList *));

  if (!first_list || !second_list)
  {
    print_error(module_name, NULL, "memory allocation error");
    return 1;
  }

  sll_init(first_list);
  sll_init(second_list);

  read_dir_to_list(first_dir_name, first_list);
  read_dir_to_list(second_dir_name, second_list);

  cmp_files_lists_with_log(first_list, second_list, output_file_name);

  sll_terminate(first_list);
  sll_terminate(second_list);

  free(first_list);
  free(second_list);

  return 0;
}

bool is_dir(const char *dir_path)
{
  struct stat temp_stat;

  if (lstat(dir_path, &temp_stat) == -1)
  {
    print_error(module_name, NULL, strerror(errno));
    return false;
  }

  return S_ISDIR(temp_stat.st_mode);
}

void print_error(const char *module_name, const char* additional_info, const char *error_msg)
{
  if (additional_info)
  {
    fprintf(stderr, "%s: %s: %s\n", module_name, additional_info, error_msg);
  }
  else
  {
    fprintf(stderr, "%s: %s\n", module_name, error_msg);
  }
}

int read_dir_to_list(const char *curr_dir_path, struct SLList *source_list)
{
  DIR *curr_dir = opendir(curr_dir_path);

  if (!curr_dir)
  {
    print_error(module_name, curr_dir_path, strerror(errno));
    return 1;
  }

  struct dirent *temp_dirent;

  while ((temp_dirent = readdir(curr_dir)) != NULL)
  {
    if (!strcmp(".", temp_dirent->d_name) || !strcmp("..", temp_dirent->d_name))
    {
      continue;
    }

    if (temp_dirent->d_type == DT_REG)
    {
      char full_path[PATH_MAX];
      char unresovled_path[PATH_MAX];

      strcpy(unresovled_path, curr_dir_path);
      strcat(unresovled_path, "/");
      strcat(unresovled_path, temp_dirent->d_name);

      realpath(unresovled_path, full_path);

      sll_push(source_list, full_path);
    }
    else if (temp_dirent->d_type == DT_DIR)
    {
      char new_dir_path[PATH_MAX];

      strcpy(new_dir_path, curr_dir_path);
      strcat(new_dir_path, "/");
      strcat(new_dir_path, temp_dirent->d_name);

      read_dir_to_list(new_dir_path, source_list);
    }
  }

  //check readdir return value
  if (errno)
  {
    print_error(module_name, curr_dir_path, strerror(errno));
    return 1;
  }

  if (closedir(curr_dir) == -1)
  {
    print_error(module_name, curr_dir_path, strerror(errno));
    return 1;
  }

  return 0;
}

int filecmp(const char *path1, const char *path2)
{
  const int READ_BLOCK = 4096;

  int fd1 = open(path1, O_RDONLY);
  if (fd1 == -1)
  {
    print_error(module_name, path1, strerror(errno));
    return 1;
  }

  int fd2 = open(path2, O_RDONLY);
  if (fd2 == -1)
  {
    print_error(module_name, path2, strerror(errno));
    return 1;
  }

  char buf1[READ_BLOCK + 1];
  char buf2[READ_BLOCK + 1];

  int read_bytes1;
  int read_bytes2;

  while ((read_bytes1 = read(fd1, buf1, READ_BLOCK)) > 0 && (read_bytes2 = read(fd2, buf2, READ_BLOCK)) > 0)
  {
    buf1[read_bytes1] = '\0';
    buf2[read_bytes2] = '\0';

    if (read_bytes1 != read_bytes2)
    {
      close(fd1);
      close(fd2);

      return 1;
    }

    if (strcmp(buf1, buf2) != 0)
    {
      close(fd1);
      close(fd2);

      return 1;
    }
  }

  close(fd1);
  close(fd2);

  return 0;
}

int log_file_info(const char *file_path, const char *outputfile_path)
{
  int output_file = open(outputfile_path, O_WRONLY | O_CREAT | O_APPEND, 0664);

  if (output_file == -1)
  {
    print_error(module_name, outputfile_path, strerror(errno));
    return 1;
  }

  const int WRITE_BLOCK = 8192;
  const int PERMS_BLOCK = 9;

  struct stat file_stat;

  if (lstat(file_path, &file_stat) == -1)
  {
    print_error(module_name, file_path, strerror(errno));
    close(output_file);
    return 1;
  }

  char file_perms[PERMS_BLOCK + 1];

  file_perms[0] = file_stat.st_mode & S_IRUSR ? 'r' : '-';
  file_perms[1] = file_stat.st_mode & S_IWUSR ? 'w' : '-';
  file_perms[2] = file_stat.st_mode & S_IXUSR ? 'x' : '-';
  file_perms[3] = file_stat.st_mode & S_IRGRP ? 'r' : '-';
  file_perms[4] = file_stat.st_mode & S_IWGRP ? 'w' : '-';
  file_perms[5] = file_stat.st_mode & S_IXGRP ? 'x' : '-';
  file_perms[6] = file_stat.st_mode & S_IROTH ? 'r' : '-';
  file_perms[7] = file_stat.st_mode & S_IWOTH ? 'w' : '-';
  file_perms[8] = file_stat.st_mode & S_IXOTH ? 'x' : '-';
  file_perms[9] = '\0';

  char file_info[WRITE_BLOCK];
  char temp_buf[WRITE_BLOCK / 2];

  sprintf(temp_buf, "%s ", file_path);
  strcpy(file_info, temp_buf);
  sprintf(temp_buf, "%lld ", (long long)file_stat.st_size);
  strcat(file_info, temp_buf);
  sprintf(temp_buf, "%s ", ctime(&file_stat.st_mtime));
  strcat(file_info, temp_buf);
  sprintf(temp_buf, "%s ", file_perms);
  strcat(file_info, temp_buf);
  sprintf(temp_buf, "%ld \n", (long)file_stat.st_ino);
  strcat(file_info, temp_buf);
  
  if (write(output_file, file_info, strlen(file_info)) == -1)
  {
    print_error(module_name, outputfile_path, strerror(errno));

    close(output_file);
    return 1;
  }

  close(output_file);

  return 0;
}

void cmp_files_lists_with_log(struct SLList *first_list, struct SLList *second_list, const char *log_file)
{
  struct Node *first_temp = first_list->head;

  while (first_temp->next != NULL)
  {
    struct Node *second_temp = second_list->head;

    while (second_temp->next != NULL)
    {
      if (!filecmp(first_temp->next->value, second_temp->next->value))
      {
        log_file_info(first_temp->next->value, log_file);

        log_file_info(second_temp->next->value, log_file);
      }

      second_temp = second_temp->next;
    }

    first_temp = first_temp->next;
  }
}
