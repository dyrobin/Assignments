#include <stdio.h>
#include <string.h>


struct email_db {
  char stuno[16];
  char email[128];
};

struct group_db {
  char stuno1[16];
  char name1[128];
  char stuno2[16];
  char name2[128];
};

struct cred_db {
  char pass[16];
  char acc[64];
  char sec[64];
};

struct email_db emails[60];
int n_emails = 0;

struct group_db groups[60];
int n_groups = 0;

struct cred_db creds[60];

char empty[] = "";

char *lookup_email(const char *s) {
  int i;
  for (i = 0;i < n_emails;i ++) {
    if (strcmp(s, emails[i].stuno) == 0) {
      return emails[i].email;
    }
  }
  return empty;
}

int main(int argc, char *argv[])
{
  FILE *fp;
  char lbuf[512];
  char *p_comma, *sno;
  int i;
  int gno;

  memset(emails,0,sizeof(struct email_db)*60);
  memset(groups,0,sizeof(struct group_db)*60);
  memset(creds,0,sizeof(struct cred_db)*60);

  /* emails */
  fp = fopen("emails.csv", "r");
  /* format:  <student no>,<email>\n */
  while (fgets(lbuf, 512, fp) != NULL) {
    p_comma = strchr(lbuf, ',');
    *p_comma = ' ';
    sscanf(lbuf, "%s%s", emails[n_emails].stuno, emails[n_emails].email);
    n_emails++;
  }
  /*
  printf("n_emails: %d\n", n_emails);
  for (i = 0;i < n_emails;i ++) {
    printf("%d: stuno:%s email:%s\n", i, emails[i].stuno, emails[i].email);
  }
  */
  fclose(fp);

  /* groups */
  /* format: <group no>,<name1>,<stuno1>,<name2>,<stuno2>\n */
  fp = fopen("groups.csv", "r");
  while (fgets(lbuf, 512, fp) != NULL) {
    sscanf(lbuf, "%d", &gno);
    p_comma = strchr(lbuf, ',');
    sno = strchr(p_comma+1, ',');
    p_comma = strchr(sno+1, ',');
    if (p_comma != NULL) *p_comma = ' ';
    sscanf(sno+1, "%s", groups[gno].stuno1);

    if (p_comma != NULL) {
      sno = strchr(p_comma+1, ',');
      p_comma = strchr(sno+1, ',');
      if (p_comma != NULL) *p_comma = ' ';
      sscanf(sno+1, "%s", groups[gno].stuno2);
    }
    if (gno > n_groups) n_groups = gno;
  }
  /*
  for (i = 1;i <= n_groups;i ++) {
    printf("%d: stuno1:%s(%s) stuno2:%s(%s)\n", i,
     groups[i].stuno1, lookup_email(groups[i].stuno1),
     groups[i].stuno2, lookup_email(groups[i].stuno2));
  }
  */
  fclose(fp);

  fp = fopen("keys.csv", "r");
  /* format: <groupno>,<access key id>,<secret access key>,<password>\n */
  while (fgets(lbuf, 512, fp) != NULL) {
    sscanf(lbuf, "%d", &gno);
    sno = strchr(lbuf, ',');
    p_comma = strchr(sno+1, ',');
    *p_comma = ' ';
    sscanf(sno+1, "%s", creds[gno].acc);
    sno = p_comma;
    p_comma = strchr(sno+1, ',');
    *p_comma = ' ';
    sscanf(sno+1, "%s", creds[gno].sec);
    sno = p_comma;
    sscanf(sno+1, "%s", creds[gno].pass);
  }
  /*
  for (i = 1;i <= n_groups;i ++) {
    printf("%d: p:%s ac:%s se:%s\n", i, creds[i].pass, creds[i].acc, creds[i].sec);
  }
  */
  fclose(fp);

  for (i = 1;i <= n_groups;i ++) {
   printf("======= GROUP %d ========\n", i);
   printf("%s,%s\n", lookup_email(groups[i].stuno1), lookup_email(groups[i].stuno2));
   printf("\n");
   printf("Username: group%02d\n", i);
   printf("Password: %s\n", creds[i].pass);
   printf("Access Key ID: %s\n", creds[i].acc);
   printf("Secret Access Key: %s\n", creds[i].sec);
   printf("\n");
  }
  return 0;
}

