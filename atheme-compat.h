/*
 * Copyright (C) 2012 William Pitcock <nenolod@dereferenced.org>
 * Copyright (C) 2018 Atheme Development Group (https://atheme.github.io/)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ATHEME_CONTRIB_COMPAT_H
#define ATHEME_CONTRIB_COMPAT_H 1

#include "atheme.h"
#include "contrib-vendors.h"



#ifndef STR_EMAIL_NOT_VERIFIED
#  define STR_EMAIL_NOT_VERIFIED        _("You must verify your e-mail address before you may perform this operation.")
#endif
#ifndef STR_INSUFFICIENT_PARAMS
#  define STR_INSUFFICIENT_PARAMS       _("Insufficient parameters for \2%s\2.")
#endif
#ifndef STR_INVALID_PARAMS
#  define STR_INVALID_PARAMS            _("Invalid parameters for \2%s\2.")
#endif
#ifndef STR_IS_NOT_REGISTERED
#  define STR_IS_NOT_REGISTERED         _("\2%s\2 is not registered.")
#endif
#ifndef STR_NO_PRIVILEGE
#  define STR_NO_PRIVILEGE              _("You do not have the \2%s\2 privilege.")
#endif
#ifndef STR_NOT_AUTHORIZED
#  define STR_NOT_AUTHORIZED            _("You are not authorized to perform this operation.")
#endif
#ifndef STR_NOT_AUTHORIZED_TARGET
#  define STR_NOT_AUTHORIZED_TARGET     _("You are not authorized to use the target argument.")
#endif
#ifndef STR_NOT_LOGGED_IN
#  define STR_NOT_LOGGED_IN             _("You are not logged in.")
#endif



#if (CURRENT_ABI_REVISION < 710000)

typedef char *stringref;

#  define compat_sendemail(user, myuser, type, email, content)  \
              sendemail(user, type, myuser, content)

#else /* (CURRENT_ABI_REVISION < 710000) */
#  define compat_sendemail      sendemail
#endif /* (CURRENT_ABI_REVISION >= 710000) */



#if (CURRENT_ABI_REVISION < 730000)

/* This function was introduced in the Atheme 7.3 development series,
 * and both crypt_string() and gen_salt() were removed. We want to use
 * the new function, but if we're building against older versions,
 * redefine it in terms of the older functions.
 */
#  define crypt_password(pass)                  crypt_string(pass, gen_salt())

/* These were renamed in the Atheme 7.3 development series and we want
 * to use the new names.
 */
#  define atheme_object(obj)                    object(obj)
#  define atheme_object_dispose(obj)            object_dispose(obj)
#  define atheme_object_init(obj, name, dest)   object_init(obj, name, dest)
#  define atheme_object_ref(obj)                object_ref(obj)
#  define atheme_object_sink_ref(obj)           object_sink_ref(obj)
#  define atheme_object_unref(obj)              object_unref(obj)
#  define atheme_random()                       arc4random()
#  define atheme_random_uniform(bound)          arc4random_uniform(bound)
#  define atheme_random_buf(dest, size)         arc4random_buf(dest, size)

/* This function was introduced in the Atheme 7.3 development series and
 * we want to make use of it.
 */
#  define sfree(ptr)                            free(ptr)

/* These attribute macros were introduced in the Atheme 7.3 development
 * series, and it would be nice to be able to make use of them.
 */
#  if defined(__GNUC__) || defined(__INTEL_COMPILER)
#    define ATHEME_FATTR_DEPRECATED             __attribute__((deprecated)
#    define ATHEME_FATTR_MALLOC                 __attribute__((malloc))
#    define ATHEME_FATTR_NORETURN               __attribute__((noreturn))
#    define ATHEME_FATTR_PRINTF(fmt, start)     __attribute__((format(__printf__, fmt, start)))
#    define ATHEME_FATTR_SCANF(fmt, start)      __attribute__((format(__scanf__, fmt, start)))
#    define ATHEME_FATTR_WUR                    __attribute__((warn_unused_result))
#    define ATHEME_SATTR_PACKED                 __attribute__((packed))
#    define ATHEME_VATTR_ALIGNED(alignment)     __attribute__((aligned(alignment)))
#    define ATHEME_VATTR_UNUSED                 __attribute__((unused))
#  else
#    define ATHEME_FATTR_DEPRECATED             /* No function attribute support */
#    define ATHEME_FATTR_MALLOC                 /* No function attribute support */
#    define ATHEME_FATTR_NORETURN               /* No function attribute support */
#    define ATHEME_FATTR_PRINTF(fmt, start)     /* No function attribute support */
#    define ATHEME_FATTR_SCANF(fmt, start)      /* No function attribute support */
#    define ATHEME_FATTR_WUR                    /* No function attribute support */
#    define ATHEME_SATTR_PACKED                 /* No structure attribute support */
#    define ATHEME_VATTR_ALIGNED(alignment)     /* No variable attribute support */
#    define ATHEME_VATTR_UNUSED                 /* No variable attribute support */
#  endif

// This new name was introduced in the Atheme 7.3 development series.
#define MODFLAG_FAIL MODTYPE_FAIL

/* These 2 macros were introduced in the Atheme 7.3 development series and
 * they're much nicer to use.
 */
#define VENDOR_DECLARE_MODULE_V1(name, unloadcap, ven)  \
        DECLARE_MODULE_V1(name, unloadcap, mod_init, mod_deinit, PACKAGE_STRING, ven);

#define SIMPLE_DECLARE_MODULE_V1(name, unloadcap)       \
	VENDOR_DECLARE_MODULE_V1(name, unloadcap, VENDOR_STRING)

/* These were redefined in the Atheme 7.3 development series to not
 * include the space for a NULL terminator in their definition. This
 * repository now assumes this is the case, so when building against
 * 7.2 or earlier we need to subtract 1 or provide our own definition
 * for a macro newly introduced in 7.3 (GROUPLEN).
 */
#  define COMPAT_HOSTLEN        (HOSTLEN - 1)
#  define COMPAT_NICKLEN        (NICKLEN - 1)
#  define COMPAT_PASSLEN        (PASSLEN - 1)
#  define COMPAT_IDLEN          (IDLEN - 1)
#  define COMPAT_CHANNELLEN     (CHANNELLEN - 1)
#  define COMPAT_GROUPLEN       (NICKLEN - 1)
#  define COMPAT_USERLEN        (USERLEN - 1)
#  define COMPAT_HOSTIPLEN      (HOSTIPLEN - 1)
#  define COMPAT_GECOSLEN       (GECOSLEN - 1)
#  define COMPAT_KEYLEN         (KEYLEN - 1)
#  define COMPAT_EMAILLEN       (EMAILLEN - 1)
#  define COMPAT_MEMOLEN        (MEMOLEN - 1)

#else /* (CURRENT_ABI_REVISION < 730000) */

#  define COMPAT_HOSTLEN        HOSTLEN
#  define COMPAT_NICKLEN        NICKLEN
#  define COMPAT_PASSLEN        PASSLEN
#  define COMPAT_IDLEN          IDLEN
#  define COMPAT_CHANNELLEN     CHANNELLEN
#  define COMPAT_GROUPLEN       GROUPLEN
#  define COMPAT_USERLEN        USERLEN
#  define COMPAT_HOSTIPLEN      HOSTIPLEN
#  define COMPAT_GECOSLEN       GECOSLEN
#  define COMPAT_KEYLEN         KEYLEN
#  define COMPAT_EMAILLEN       EMAILLEN
#  define COMPAT_MEMOLEN        MEMOLEN

// This definition was removed in the Atheme 7.3 development series.
#  define IRCD_RES_HOSTLEN      255

// This is a no-op in at least 7.3+ and were removed.
static inline struct hook *hook_add_event(const char *name) { return NULL; }
static inline void hook_del_event(struct hook *hook) { }

/* These typedefs were removed in the Atheme 7.3 development series,
 * but we can't use the new names because they consist of structures
 * or enums and we can't make structure aliases to other structures
 * present in the older versions.
 */
typedef enum cmd_faultcode                      cmd_faultcode_t;
typedef enum database_transaction               database_transaction_t;
typedef enum db_save_strategy                   db_save_strategy_t;
typedef enum log_type                           log_type_t;
typedef enum module_unload_capability           module_unload_capability_t;
typedef enum module_unload_intent               module_unload_intent_t;

typedef struct hook                             hook_t;
typedef struct hook_channel_joinpart            hook_channel_joinpart_t;
typedef struct hook_channel_message             hook_cmessage_data_t;
typedef struct hook_channel_mode                hook_channel_mode_t;
typedef struct hook_channel_mode_change         hook_channel_mode_change_t;
typedef struct hook_channel_topic_check         hook_channel_topic_check_t;
typedef struct hook_server_delete               hook_server_delete_t;
typedef struct hook_user_nick                   hook_user_nick_t;
typedef struct hook_user_delete_info            hook_user_delete_t;

typedef struct hook_channel_acl_req             hook_channel_acl_req_t;
typedef struct hook_channel_register_check      hook_channel_register_check_t;
typedef struct hook_channel_req                 hook_channel_req_t;
typedef struct hook_channel_successtion_req     hook_channel_succession_req_t;
typedef struct hook_expiry_req                  hook_expiry_req_t;
typedef struct hook_host_request                hook_host_request_t;
typedef struct hook_info_noexist                hook_info_noexist_req;
typedef struct hook_metadata_change             hook_metadata_change_t;
typedef struct hook_module_load                 hook_module_load_t;
typedef struct hook_myentity_req                hook_myentity_req_t;
typedef struct hook_nick_enforce                hook_nick_enforce_t;
typedef struct hook_user_needforce              hook_user_needforce_t;
typedef struct hook_user_register_check         hook_user_register_check_t;
typedef struct hook_user_rename                 hook_user_rename_t;
typedef struct hook_user_req                    hook_user_req_t;
typedef struct hook_user_login_check            hook_user_login_check_t;
typedef struct hook_user_logout_check           hook_user_logout_check_t;
typedef struct hook_user_rename_check           hook_user_rename_check_t;

typedef struct atheme_regex                     atheme_regex_t;
typedef struct authcookie                       authcookie_t;
typedef struct chanacs                          chanacs_t;
typedef struct chanban                          chanban_t;
typedef struct channel                          channel_t;
typedef struct chansvs                          chansvs_t;
typedef struct chanuser                         chanuser_t;
typedef struct claro_state                      claro_state_t;
typedef struct command                          command_t;
typedef struct connection                       connection_t;
typedef struct crypt_impl                       crypt_impl_t;
typedef struct database_handle                  database_handle_t;
typedef struct database_module                  database_module_t;
typedef struct database_vtable                  database_vtable_t;
typedef struct default_template                 default_template_t;
typedef struct res_dns_query                    dns_query_t;
typedef struct res_dns_reply                    dns_reply_t;
typedef struct email_canonicalizer_item         email_canonicalizer_item_t;
typedef struct entity_chanacs_validation_vtable entity_chanacs_validation_vtable_t;
typedef struct groupacs                         groupacs_t;
typedef struct ircd                             ircd_t;
typedef struct kline                            kline_t;
typedef struct language                         language_t;
typedef struct logfile                          logfile_t;
typedef struct metadata                         metadata_t;
typedef struct module                           module_t;
typedef struct mycertfp                         mycertfp_t;
typedef struct mychan                           mychan_t;
typedef struct myentity                         myentity_t;
typedef struct mygroup                          mygroup_t;
typedef struct mymemo                           mymemo_t;
typedef struct mynick                           mynick_t;
typedef struct myuser                           myuser_t;
typedef struct myuser_name                      myuser_name_t;
typedef struct nicksvs                          nicksvs_t;
typedef struct nsaddr                           nsaddr_t;
typedef struct atheme_object                    object_t;
typedef struct operclass                        operclass_t;
typedef struct path_handler                     path_handler_t;
typedef struct proto_cmd                        pcommand_t;
typedef struct qline                            qline_t;
typedef struct sasl_message                     sasl_message_t;
typedef struct server                           server_t;
typedef struct service                          service_t;
typedef struct soper                            soper_t;
typedef struct sourceinfo                       sourceinfo_t;
typedef struct svsignore                        svsignore_t;
typedef struct atheme_table                     table_t;
typedef struct atheme_table_cell                table_cell_t;
typedef struct atheme_table_row                 table_row_t;
typedef struct taint_reason                     taint_reason_t;
typedef struct tld                              tld_t;
typedef struct uid_provider                     uid_provider_t;
typedef struct uplink                           uplink_t;
typedef struct user                             user_t;
typedef struct v4_moduleheader                  v4_moduleheader_t;
typedef struct xline                            xline_t;

typedef conf_handler_fn                         conf_handler_t;
typedef atheme_object_destructor_fn             destructor_t;
typedef email_canonicalizer_fn                  email_canonicalizer_t;
typedef hook_fn                                 hookfn_t;
typedef log_write_func_fn                       log_write_func_t;
typedef void                                  (*module_unload_handler_t)(module_t *, module_unload_intent_t);

#endif /* (CURRENT_ABI_REVISION >= 730000) */



#endif /* !ATHEME_CONTRIB_COMPAT_H */
