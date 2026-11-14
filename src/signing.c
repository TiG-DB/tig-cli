/* Signs commits with an SSH key by running ssh-keygen, as COMMITS.md describes, so libtig needs no crypto. */
#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static long read_file(const char *path, char *buffer, size_t size) {
  FILE *file = fopen(path, "rb");
  size_t length;

  if (file == NULL) {
    return -1;
  }
  length = fread(buffer, 1, size - 1, file);
  buffer[length] = '\0';
  fclose(file);
  return (long)length;
}

static tig_status sign(void *context, const char *content, size_t content_length, char *signature,
                       size_t signature_size) {
  const cli_signer *signer = (const cli_signer *)context;
  char directory[1024];
  char payload_path[1100];
  char signature_path[1110];
  char output[1024];
  const char *tmp = getenv("TMPDIR");
  tig_status result = TIG_ERROR_REJECTED;
  FILE *payload;

  output[0] = '\0';
  snprintf(directory, sizeof(directory), "%s/tig-sign-XXXXXX", tmp != NULL && *tmp != '\0' ? tmp : "/tmp");
  if (mkdtemp(directory) == NULL) {
    return TIG_ERROR_REJECTED;
  }
  snprintf(payload_path, sizeof(payload_path), "%s/payload", directory);
  snprintf(signature_path, sizeof(signature_path), "%s.sig", payload_path);

  payload = fopen(payload_path, "wb");
  if (payload != NULL) {
    const char *argv[] = {"ssh-keygen", "-q", "-Y", "sign", "-n", "git", "-f", signer->key_path, payload_path, NULL};
    int wrote = fwrite(content, 1, content_length, payload) == content_length;

    if (fclose(payload) == 0 && wrote && cli_run(argv, output, sizeof(output)) == 0 &&
        read_file(signature_path, signature, signature_size) > 0) {
      result = TIG_OK;
    } else {
      fprintf(stderr, "ssh-keygen -Y sign failed: %s\n", output);
    }
  }
  unlink(signature_path);
  unlink(payload_path);
  rmdir(directory);
  return result;
}

int cli_signer_load(cli_signer *signer, const char *key, char *error, size_t error_size) {
  char public_path[1100];
  char public_key[1024];
  char output[1024];
  const char *argv[] = {"ssh-keygen", "-l", "-E", "sha256", "-f", public_path, NULL};
  size_t length = strlen(key);
  const char *key_start;
  size_t type_length;
  size_t key_length;
  const char *fingerprint;
  size_t fingerprint_length;

  memset(signer, 0, sizeof(*signer));
  if (length >= sizeof(signer->key_path) - 5) {
    snprintf(error, error_size, "key path is too long");
    return -1;
  }
  strcpy(signer->key_path, key);
  if (length > 4 && strcmp(key + length - 4, ".pub") == 0) {
    strcpy(public_path, key);
  } else {
    snprintf(public_path, sizeof(public_path), "%s.pub", key);
  }

  if (read_file(public_path, public_key, sizeof(public_key)) <= 0) {
    snprintf(error, error_size, "cannot read the public key %s", public_path);
    return -1;
  }
  type_length = strcspn(public_key, " ");
  if (strncmp(public_key, "ssh-ed25519 ", 12) != 0) {
    snprintf(error, error_size, "%s is not an Ed25519 public key", public_path);
    return -1;
  }
  key_start = public_key + type_length + 1;
  key_length = strcspn(key_start, " \t\r\n");
  if (key_length == 0) {
    snprintf(error, error_size, "%s is malformed", public_path);
    return -1;
  }

  if (cli_run(argv, output, sizeof(output)) != 0 || (fingerprint = strchr(output, ' ')) == NULL) {
    snprintf(error, error_size, "cannot fingerprint %s", public_path);
    return -1;
  }
  ++fingerprint;
  fingerprint_length = strcspn(fingerprint, " \t\r\n");
  if (strncmp(fingerprint, "SHA256:", 7) != 0) {
    snprintf(error, error_size, "cannot fingerprint %s", public_path);
    return -1;
  }

  if (snprintf(signer->allowed_signer, sizeof(signer->allowed_signer), "%.*s namespaces=\"git\" ssh-ed25519 %.*s",
               (int)fingerprint_length, fingerprint, (int)key_length, key_start) >= (int)sizeof(signer->allowed_signer)) {
    snprintf(error, error_size, "%s is too long", public_path);
    return -1;
  }
  signer->device.allowed_signer = signer->allowed_signer;
  signer->device.sign = sign;
  signer->device.sign_context = signer;
  return 0;
}
