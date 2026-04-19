#include "menu.h"
#include "data.h"
#include "auth.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ==================== 前向声明 ==================== */
static void create_doctor_archive(Database *db, const char *dataDir);
static void delete_doctor(Database *db, const char *dataDir);
/* 以下函数在 menu_part1.c 中实现，已在 menu.h 中声明 */
/* void add_patient(Database *db, const char *dataDir); */
/* void delete_patient(Database *db, const char *dataDir); */
void link_archive_to_account(Database *db, const char *dataDir);
void add_archive(Database *db, const char *dataDir);
void delete_archive(Database *db, const char *dataDir);
void edit_archive(Database *db, const char *dataDir);
void edit_patient(Database *db, const char *dataDir);
static void edit_doctor(Database *db, const char *dataDir);

/* ==================== 用户账号管理菜单 ==================== */
void user_account_management_menu(Database *db, const char *dataDir)
{
    (void)dataDir;
    if (!check_permission(ROLE_MANAGER))
    {
        printf("权限不足！\n");
        pause_and_wait();
        return;
    }

    while (1)
    {
        printf("\n\n============================================\n");
        printf("         用户账号管理\n");
        printf("============================================\n");
        printf("1. 查看所有账号\n");
        printf("2. 新增账号\n");
        printf("3. 删除账号\n");
        printf("4. 修改密码\n");
        printf("0. 返回上级菜单\n");
        printf("============================================\n");
        printf("请选择：");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            while (getchar() != '\n');
            continue;
        }

        switch (choice)
        {
            case 1:
            {
                printf("\n--- 所有账号列表 ---\n");
                Account *acc = db->accounts;
                if (!acc)
                {
                    printf("暂无账号记录。\n");
                }
                else
                {
                    printf("%-20s %-15s %-10s\n", "用户名", "角色", "关联ID");
                    printf("--------------------------------------------\n");
                    while (acc)
                    {
                        printf("%-20s %-15s %-10d\n", acc->username, get_role_name(acc->role), acc->linkedId);
                        acc = acc->next;
                    }
                }
                pause_and_wait();
                break;
            }
            case 2:
            {
                char username[32], password[64];
                int roleVal, linkedId;

                printf("\n--- 新增账号 ---\n");
                printf("用户名：");
                scanf("%31s", username);

                if (find_account(db, username))
                {
                    printf("错误：用户名已存在！\n");
                    pause_and_wait();
                    break;
                }

                printf("密码：");
                scanf("%63s", password);

                printf("角色（0-患者，1-医生，2-管理员）：");
                if (scanf("%d", &roleVal) != 1 || roleVal < 0 || roleVal > 2)
                {
                    printf("无效的角色选择！\n");
                    pause_and_wait();
                    break;
                }

                printf("关联ID（患者病历号/医生工号/管理员填0）：");
                if (scanf("%d", &linkedId) != 1)
                {
                    printf("无效的关联ID！\n");
                    pause_and_wait();
                    break;
                }

                if (create_account(db, username, password, (UserRole)roleVal, linkedId))
                {
                    printf("账号创建成功！\n");
                    save_all(db, dataDir);
                }
                else
                {
                    printf("账号创建失败！\n");
                }
                pause_and_wait();
                break;
            }
            case 3:
            {
                char username[32];
                printf("\n--- 删除账号 ---\n");
                printf("请输入要删除的用户名：");
                scanf("%31s", username);

                Account *acc = find_account(db, username);
                if (!acc)
                {
                    printf("未找到该用户！\n");
                    pause_and_wait();
                    break;
                }

                // 不允许删除自己
                if (strcmp(acc->username, g_session.username) == 0)
                {
                    printf("不能删除当前登录的账号！\n");
                    pause_and_wait();
                    break;
                }

                // 从链表中移除
                if (db->accounts == acc)
                {
                    db->accounts = acc->next;
                }
                else
                {
                    Account *prev = db->accounts;
                    while (prev && prev->next != acc)
                        prev = prev->next;
                    if (prev)
                        prev->next = acc->next;
                }
                // 只释放当前节点，不要调用 free_accounts 因为它会释放整个后续链表
                free(acc);
                printf("账号 %s 已删除！\n", username);
                save_all(db, dataDir);
                pause_and_wait();
                break;
            }
            case 4:
            {
                char username[32], newPassword[64];
                printf("\n--- 修改密码 ---\n");
                printf("请输入用户名：");
                scanf("%31s", username);

                Account *acc = find_account(db, username);
                if (!acc)
                {
                    printf("未找到该用户！\n");
                    pause_and_wait();
                    break;
                }

                printf("请输入新密码：");
                scanf("%63s", newPassword);

                strncpy(acc->password, newPassword, sizeof(acc->password) - 1);
                acc->password[sizeof(acc->password) - 1] = '\0';

                printf("密码修改成功！\n");
                save_all(db, dataDir);
                pause_and_wait();
                break;
            }
            case 0:
                return;
            default:
                printf("无效选择！\n");
                pause_and_wait();
                break;
        }
    }
}

/* ==================== 档案管理辅助函数 ==================== */

/*
 * 说明：创建医生档案
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
static void create_doctor_archive(Database *db, const char *dataDir) {
    Doctor *d = (Doctor*)malloc(sizeof(Doctor));
    if (!d) {
        printf("内存分配失败！\n");
        pause_and_wait();
        return;
    }
    d->id = next_doctor_id(db);
    printf("自动生成的工号：%d\n", d->id);
    
    read_line("姓名：", d->name, sizeof(d->name));
    read_line("科室：", d->dept, sizeof(d->dept));
    read_line("职称：", d->title, sizeof(d->title));
    
    d->archived = 0;
    d->next = NULL;
    
    // 追加到链表
    if (!db->doctors)
        db->doctors = d;
    else {
        Doctor *cur = db->doctors;
        while (cur->next) cur = cur->next;
        cur->next = d;
    }
    
    printf("医生档案创建成功！工号：%d\n", d->id);
    save_all(db, dataDir);
}

/*
 * 说明：关联档案到账号
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
void link_archive_to_account(Database *db, const char *dataDir) {
    char username[32];
    int targetId;
    printf("\n--- 关联档案到账号 ---\n");
    printf("请输入要关联档案的用户名：");
    scanf("%31s", username);

    Account *acc = find_account(db, username);
    if (!acc) {
        printf("未找到该用户！\n");
        pause_and_wait();
        return;
    }

    if (acc->role == ROLE_MANAGER) {
        printf("管理员账号不需要关联档案。\n");
        pause_and_wait();
        return;
    }

    printf("当前关联 ID：%d\n", acc->linkedId);
    
    if (acc->role == ROLE_PATIENT) {
        printf("请输入要关联的患者病历号：");
        if (scanf("%d", &targetId) != 1) {
            printf("无效输入！\n");
            pause_and_wait();
            return;
        }
        
        Patient *p = find_patient(db, targetId);
        if (!p) {
            printf("未找到该病历号对应的患者档案！\n");
            pause_and_wait();
            return;
        }
        
        if (acc->linkedId != 0 && acc->linkedId != targetId) {
            printf("警告：该账号已关联其他患者档案（原病历号：%d）。\n", acc->linkedId);
            printf("确认要重新关联吗？(y/n): ");
            char confirm[16];
            scanf("%15s", confirm);
            if (confirm[0] != 'y' && confirm[0] != 'Y') {
                printf("已取消操作。\n");
                pause_and_wait();
                return;
            }
        }
        
        acc->linkedId = targetId;
        printf("已将患者账号 %s 关联到病历号 %d。\n", username, targetId);
    } else if (acc->role == ROLE_DOCTOR) {
        printf("请输入要关联的医生工号：");
        if (scanf("%d", &targetId) != 1) {
            printf("无效输入！\n");
            pause_and_wait();
            return;
        }
        
        Doctor *d = find_doctor(db, targetId);
        if (!d) {
            printf("未找到该工号对应的医生档案！\n");
            pause_and_wait();
            return;
        }
        
        if (acc->linkedId != 0 && acc->linkedId != targetId) {
            printf("警告：该账号已关联其他医生档案（原工号：%d）。\n", acc->linkedId);
            printf("确认要重新关联吗？(y/n): ");
            char confirm[16];
            scanf("%15s", confirm);
            if (confirm[0] != 'y' && confirm[0] != 'Y') {
                printf("已取消操作。\n");
                pause_and_wait();
                return;
            }
        }
        
        acc->linkedId = targetId;
        printf("已将医生账号 %s 关联到工号 %d。\n", username, targetId);
    }
    
    save_all(db, dataDir);
}

/*
 * 说明：新增医生或患者档案
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
void add_archive(Database *db, const char *dataDir) {
    int choice;
    printf("\n--- 新增档案 ---\n");
    printf("1. 新增患者档案\n");
    printf("2. 新增医生档案\n");
    printf("0. 返回上一步\n");
    choice = read_int("请选择：", 0, 2);
    
    if (choice == 0) return;
    else if (choice == 1) add_patient(db, dataDir);
    else if (choice == 2) create_doctor_archive(db, dataDir);
}

/*
 * 说明：删除档案（患者或医生）
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
void delete_archive(Database *db, const char *dataDir) {
    int choice;
    printf("\n--- 删除档案 ---\n");
    printf("1. 删除患者档案\n");
    printf("2. 删除医生档案\n");
    printf("0. 返回上一步\n");
    choice = read_int("请选择：", 0, 2);
    
    if (choice == 0) return;
    else if (choice == 1) delete_patient(db, dataDir);
    else if (choice == 2) delete_doctor(db, dataDir);
}

/*
 * 说明：修改档案（患者或医生）
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
void edit_archive(Database *db, const char *dataDir) {
    int choice;
    printf("\n--- 修改档案 ---\n");
    printf("1. 修改患者档案\n");
    printf("2. 修改医生档案\n");
    printf("0. 返回上一步\n");
    choice = read_int("请选择：", 0, 2);
    
    if (choice == 0) return;
    else if (choice == 1) edit_patient(db, dataDir);
    else if (choice == 2) edit_doctor(db, dataDir);
}

/*
 * 说明：删除医生档案
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
static void delete_doctor(Database *db, const char *dataDir) {
    int id = read_int("要删除的医生工号 (输入 0 返回): ", 0, 1000000);
    Doctor *prev = NULL;
    Doctor *cur = db->doctors;
    char confirm[16];
    
    if (id == 0) { 
        printf("已返回上一步。\n"); 
        return; 
    }

    while (cur && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }

    if (!cur) {
        printf("医生不存在。\n");
        return;
    }

    /* 检查医生是否有关联的挂号记录 */
    Registration *r;
    for (r = db->registrations; r; r = r->next) {
        if (r->doctorId == id) {
            printf("删除失败：该医生存在关联的挂号记录，请先处理关联数据。\n");
            return;
        }
    }

    printf("确认删除医生 [%d] %s ? (y/n): ", cur->id, cur->name);
    read_line(NULL, confirm, sizeof(confirm));
    if (!(confirm[0] == 'y' || confirm[0] == 'Y')) {
        printf("已取消删除。\n");
        return;
    }

    if (prev) prev->next = cur->next;
    else db->doctors = cur->next;
    free(cur);
    save_all(db, dataDir);
    printf("删除成功。\n");
}

/*
 * 说明：修改患者档案
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
void edit_patient(Database *db, const char *dataDir) {
    int id = read_int("请输入要修改的患者病历号 (输入 0 返回): ", 0, 1000000);
    Patient *p;
    char newName[NAME_LEN], newGender[16], newBirth[DATE_LEN], newPhone[PHONE_LEN], newInsurance[SMALL_LEN];
    
    if (id == 0) { 
        printf("已返回上一步。\n"); 
        return; 
    }
    
    p = find_patient(db, id);
    if (!p) {
        printf("未找到该病历号对应的患者档案。\n");
        return;
    }
    
    printf("\n=== 修改患者档案 ===\n");
    printf("当前信息：\n");
    printf("  姓名：%s\n", p->name);
    printf("  性别：%s\n", p->gender);
    printf("  出生日期：%s\n", p->birth);
    printf("  联系电话：%s\n", p->phone);
    printf("  医保类型：%s\n", p->insurance);
    
    printf("\n请输入新信息（直接回车保持原值，输入 0 取消修改）：\n");
    
    /* 修改姓名 */
    printf("姓名 [%s]: ", p->name);
    read_line("", newName, sizeof(newName));
    if (strcmp(newName, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newName) > 0) {
        safe_copy(p->name, newName, sizeof(p->name));
    }
    
    /* 修改性别 */
    printf("性别 [%s]: ", p->gender);
    read_line("", newGender, sizeof(newGender));
    if (strcmp(newGender, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newGender) > 0) {
        if (strcmp(newGender, "男") != 0 && strcmp(newGender, "女") != 0) {
            printf("性别必须为\"男\"或\"女\"，已保持原值。\n");
        } else {
            safe_copy(p->gender, newGender, sizeof(p->gender));
        }
    }
    
    /* 修改出生日期 */
    printf("出生日期 [%s]: ", p->birth);
    read_line("", newBirth, sizeof(newBirth));
    if (strcmp(newBirth, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newBirth) > 0) {
        if (strlen(newBirth) == 10 && newBirth[4] == '-' && newBirth[7] == '-') {
            safe_copy(p->birth, newBirth, sizeof(p->birth));
        } else {
            printf("日期格式应为 YYYY-MM-DD，已保持原值。\n");
        }
    }
    
    /* 修改联系电话 */
    printf("联系电话 [%s]: ", p->phone);
    read_line("", newPhone, sizeof(newPhone));
    if (strcmp(newPhone, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newPhone) > 0) {
        int valid = 1;
        if (strlen(newPhone) != 11) {
            valid = 0;
        } else {
            for (int i = 0; i < 11; i++) {
                if (!isdigit(newPhone[i])) {
                    valid = 0;
                    break;
                }
            }
        }
        if (valid) {
            safe_copy(p->phone, newPhone, sizeof(p->phone));
        } else {
            printf("手机号应为 11 位数字，已保持原值。\n");
        }
    }
    
    /* 修改医保类型 */
    printf("医保类型 [%s]: ", p->insurance);
    read_line("", newInsurance, sizeof(newInsurance));
    if (strcmp(newInsurance, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newInsurance) > 0) {
        safe_copy(p->insurance, newInsurance, sizeof(p->insurance));
    }
    
    /* 保存修改到文件 */
    save_all(db, dataDir);
    
    printf("\n患者档案修改成功！\n");
    printf("更新后的信息：\n");
    printf("  姓名：%s\n", p->name);
    printf("  性别：%s\n", p->gender);
    printf("  出生日期：%s\n", p->birth);
    printf("  联系电话：%s\n", p->phone);
    printf("  医保类型：%s\n", p->insurance);
}

/*
 * 说明：修改医生档案
 * 参数：db 数据库指针
 * 参数：dataDir 数据文件目录
 */
static void edit_doctor(Database *db, const char *dataDir) {
    int id = read_int("请输入要修改的医生工号 (输入 0 返回): ", 0, 1000000);
    Doctor *d;
    char newName[NAME_LEN], newDept[SMALL_LEN], newTitle[SMALL_LEN];
    
    if (id == 0) { 
        printf("已返回上一步。\n"); 
        return; 
    }
    
    d = find_doctor(db, id);
    if (!d) {
        printf("未找到该工号对应的医生档案。\n");
        return;
    }
    
    printf("\n=== 修改医生档案 ===\n");
    printf("当前信息：\n");
    printf("  工号：%d\n", d->id);
    printf("  姓名：%s\n", d->name);
    printf("  科室：%s\n", d->dept);
    printf("  职称：%s\n", d->title);
    
    printf("\n请输入新信息（直接回车保持原值，输入 0 取消修改）：\n");
    
    /* 修改姓名 */
    printf("姓名 [%s]: ", d->name);
    read_line("", newName, sizeof(newName));
    if (strcmp(newName, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newName) > 0) {
        safe_copy(d->name, newName, sizeof(d->name));
    }
    
    /* 修改科室 */
    printf("科室 [%s]: ", d->dept);
    read_line("", newDept, sizeof(newDept));
    if (strcmp(newDept, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newDept) > 0) {
        safe_copy(d->dept, newDept, sizeof(d->dept));
    }
    
    /* 修改职称 */
    printf("职称 [%s]: ", d->title);
    read_line("", newTitle, sizeof(newTitle));
    if (strcmp(newTitle, "0") == 0) {
        printf("已取消修改。\n");
        return;
    }
    if (strlen(newTitle) > 0) {
        safe_copy(d->title, newTitle, sizeof(d->title));
    }
    
    /* 保存修改到文件 */
    save_all(db, dataDir);
    
    printf("\n医生档案修改成功！\n");
    printf("更新后的信息：\n");
    printf("  工号：%d\n", d->id);
    printf("  姓名：%s\n", d->name);
    printf("  科室：%s\n", d->dept);
    printf("  职称：%s\n", d->title);
}
