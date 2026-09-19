#pragma once

#include <atomic>

namespace power_timer {

// The locale is deliberately a small value type.  The GUI changes it only
// while idle; workers receive a snapshot so an in-flight operation cannot
// change language halfway through an error report.
enum class Locale { en, ru };

enum class Text {
    app_title,
    subtitle_dry_run,
    subtitle_normal,
    sleep_action,
    shutdown_action,
    through,
    hours_short,
    minutes_short,
    preset_15,
    preset_30,
    preset_1_hour,
    preset_2_hours,
    timer_not_started,
    invalid_interval,
    shutdown_detail,
    sleep_detail,
    start_timer,
    cancel_timer_button,
    timer_cancelled,
    dry_run_footer_idle,
    normal_footer_idle,
    checking_button,
    checking_status,
    checking_detail,
    action_failed,
    unknown_system_error,
    timer_start_failed,
    timer_started,
    interrupted_status,
    interrupted_detail,
    checking_trigger,
    sending_request,
    request_sending_button,
    cancellation_unavailable,
    dry_run_footer_due,
    normal_footer_due,
    remaining_30,
    dry_run_title,
    normal_title,
    dry_run_finished,
    request_submitted,
    dry_run_detail_finished,
    normal_detail_finished,
    dry_run_action_prefix,
    action_through,
    permission_error,
    sleep_unavailable,
    linux_root,
    linux_dependencies,
    linux_systemd_version,
    linux_authorization,
    linux_no_access,
    linux_system_rejected,
    linux_timeout,
    linux_timeout_followup,
    linux_no_retry,
    mac_permission,
    mac_permission_help,
    mac_restart,
    mac_rejected,
    mac_timeout,
    mac_timeout_followup,
    windows_code,
    mac_code,
    exit_code,
    process_invalid_parameters,
    help_text,
    unknown_argument,
    smoke_requires_dry_run,
    startup_error,
    language_button,
    count,
};

inline std::atomic<Locale> locale_storage{Locale::en};

inline Locale current_locale() noexcept { return locale_storage.load(std::memory_order_acquire); }

inline void set_locale(Locale locale) noexcept {
    locale_storage.store(locale, std::memory_order_release);
}

inline const char *text(Locale locale, Text key) noexcept {
    if (locale == Locale::ru) {
        switch (key) {
        case Text::count:
            return "";
        case Text::app_title:
            return "Таймер питания";
        case Text::subtitle_dry_run:
            return "Проверка без изменения питания";
        case Text::subtitle_normal:
            return "Сон или выключение по таймеру";
        case Text::sleep_action:
            return "Спящий режим";
        case Text::shutdown_action:
            return "Выключить";
        case Text::through:
            return "Через";
        case Text::hours_short:
            return "ч";
        case Text::minutes_short:
            return "мин";
        case Text::preset_15:
            return "15 мин";
        case Text::preset_30:
            return "30 мин";
        case Text::preset_1_hour:
            return "1 час";
        case Text::preset_2_hours:
            return "2 часа";
        case Text::timer_not_started:
            return "Таймер не запущен";
        case Text::invalid_interval:
            return "Укажите интервал от 1 минуты до 24 часов";
        case Text::shutdown_detail:
            return "Сохраните документы до выключения";
        case Text::sleep_detail:
            return "Спящий режим, не гибернация";
        case Text::start_timer:
            return "Запустить таймер";
        case Text::cancel_timer_button:
            return "Отменить таймер";
        case Text::timer_cancelled:
            return "Таймер отменён";
        case Text::dry_run_footer_idle:
            return "Режим проверки: сон и выключение отключены.";
        case Text::normal_footer_idle:
            return "Можно свернуть окно. Закрытие отменяет таймер.";
        case Text::checking_button:
            return "Проверка разрешений…";
        case Text::checking_status:
            return "Проверка доступности действия";
        case Text::checking_detail:
            return "После проверки начнётся отсчёт";
        case Text::action_failed:
            return "Действие не выполнено";
        case Text::unknown_system_error:
            return "Неизвестная ошибка системной операции.";
        case Text::timer_start_failed:
            return "Не удалось запустить таймер";
        case Text::timer_started:
            return "Таймер запущен";
        case Text::interrupted_status:
            return "Таймер отменён после перерыва";
        case Text::interrupted_detail:
            return "Сон, изменение часов или пауза дольше 10 секунд";
        case Text::checking_trigger:
            return "Проверка срабатывания";
        case Text::sending_request:
            return "Отправка запроса системе";
        case Text::request_sending_button:
            return "Запрос отправляется…";
        case Text::cancellation_unavailable:
            return "На этом этапе отмена уже недоступна";
        case Text::dry_run_footer_due:
            return "Режим проверки: питание не изменится.";
        case Text::normal_footer_due:
            return "Закрытие окна не отзывает отправленный запрос.";
        case Text::remaining_30:
            return "До действия осталось не больше 30 секунд";
        case Text::dry_run_title:
            return "Таймер питания — проверка";
        case Text::normal_title:
            return "Таймер питания";
        case Text::dry_run_finished:
            return "Проверка завершена";
        case Text::request_submitted:
            return "Запрос передан системе";
        case Text::dry_run_detail_finished:
            return "Сон и выключение не выполнялись";
        case Text::normal_detail_finished:
            return "Система может задержать или отклонить действие";
        case Text::dry_run_action_prefix:
            return "[Проверка] ";
        case Text::action_through:
            return " через ";
        case Text::permission_error:
            return "У текущего пользователя нет права на это действие.\n";
        case Text::sleep_unavailable:
            return "Спящий режим недоступен на этом компьютере.";
        case Text::linux_root:
            return "Запустите приложение от обычного пользователя, не через sudo.";
        case Text::linux_dependencies:
            return "Нужны systemd-logind и системные программы busctl и systemctl.\nЭта сборка не "
                   "поддерживает Linux без systemd.";
        case Text::linux_systemd_version:
            return "Нужна версия systemd 248 или новее.\n";
        case Text::linux_authorization:
            return "Для этого действия система требует дополнительную авторизацию.\nТаймер не "
                   "запущен: приложение не будет запрашивать пароль в момент "
                   "срабатывания.\nПроверьте разрешения текущего сеанса в настройках системы.";
        case Text::linux_no_access:
            return "Выбранное действие сейчас недоступно или запрещено системой.\n";
        case Text::linux_system_rejected:
            return "Система отклонила запрос. Проверьте права и запреты на сон/выключение.\n\n";
        case Text::linux_timeout:
            return "Время ожидания ответа системы истекло.\n";
        case Text::linux_timeout_followup:
            return "Если запрос уже отправлен, действие ещё может выполниться.\n";
        case Text::linux_no_retry:
            return "Автоматического повторного запроса не будет.";
        case Text::mac_permission:
            return "macOS не разрешила управление питанием.\n";
        case Text::mac_permission_help:
            return "Проверьте разрешения автоматизации приложения в системных настройках.\n";
        case Text::mac_restart:
            return "После изменения разрешений перезапустите приложение.\n";
        case Text::mac_rejected:
            return "macOS отклонила запрос. Код: ";
        case Text::mac_timeout:
            return "macOS не ответила вовремя. Запрос мог быть принят системой.\n";
        case Text::mac_timeout_followup:
            return "Автоматического повторного запроса не будет.";
        case Text::windows_code:
            return "Код Windows: ";
        case Text::mac_code:
            return "Код macOS: ";
        case Text::exit_code:
            return "Код завершения: ";
        case Text::process_invalid_parameters:
            return "Некорректные параметры запуска системной команды.";
        case Text::help_text:
            return "Таймер питания\n  --dry-run     Не вызывать API управления питанием\n  "
                   "--smoke-test  Запустить тест GUI на 2 секунды (требует --dry-run)\n  --lang "
                   "en|ru  Выбрать язык интерфейса\n  --license     Показать лицензии\n";
        case Text::unknown_argument:
            return "Неизвестный аргумент: ";
        case Text::smoke_requires_dry_run:
            return "--smoke-test требует --dry-run\n";
        case Text::startup_error:
            return "Не удалось запустить приложение.\n";
        case Text::language_button:
            return "EN";
        }
    }
    switch (key) {
    case Text::count:
        return "";
    case Text::app_title:
        return "Power Timer";
    case Text::subtitle_dry_run:
        return "Safe check without changing power state";
    case Text::subtitle_normal:
        return "Sleep or shut down on a timer";
    case Text::sleep_action:
        return "Sleep";
    case Text::shutdown_action:
        return "Shut down";
    case Text::through:
        return "In";
    case Text::hours_short:
        return "h";
    case Text::minutes_short:
        return "min";
    case Text::preset_15:
        return "15 min";
    case Text::preset_30:
        return "30 min";
    case Text::preset_1_hour:
        return "1 hour";
    case Text::preset_2_hours:
        return "2 hours";
    case Text::timer_not_started:
        return "Timer is not running";
    case Text::invalid_interval:
        return "Enter an interval from 1 minute to 24 hours";
    case Text::shutdown_detail:
        return "Save your documents before shutdown";
    case Text::sleep_detail:
        return "Sleep mode, not hibernation";
    case Text::start_timer:
        return "Start timer";
    case Text::cancel_timer_button:
        return "Cancel timer";
    case Text::timer_cancelled:
        return "Timer cancelled";
    case Text::dry_run_footer_idle:
        return "Check mode: sleep and shutdown are disabled.";
    case Text::normal_footer_idle:
        return "You can minimise the window. Closing cancels the timer.";
    case Text::checking_button:
        return "Checking permissions…";
    case Text::checking_status:
        return "Checking whether the action is available";
    case Text::checking_detail:
        return "The countdown will start after the check";
    case Text::action_failed:
        return "Action was not completed";
    case Text::unknown_system_error:
        return "Unknown system operation error.";
    case Text::timer_start_failed:
        return "Could not start the timer";
    case Text::timer_started:
        return "Timer started";
    case Text::interrupted_status:
        return "Timer cancelled after an interruption";
    case Text::interrupted_detail:
        return "Sleep, a clock change, or a pause longer than 10 seconds";
    case Text::checking_trigger:
        return "Checking the trigger";
    case Text::sending_request:
        return "Sending request to the system";
    case Text::request_sending_button:
        return "Sending request…";
    case Text::cancellation_unavailable:
        return "Cancellation is no longer available";
    case Text::dry_run_footer_due:
        return "Check mode: power state will not change.";
    case Text::normal_footer_due:
        return "Closing the window cannot recall a sent request.";
    case Text::remaining_30:
        return "30 seconds or less remain";
    case Text::dry_run_title:
        return "Power Timer — check";
    case Text::normal_title:
        return "Power Timer";
    case Text::dry_run_finished:
        return "Check completed";
    case Text::request_submitted:
        return "Request submitted to the system";
    case Text::dry_run_detail_finished:
        return "Sleep and shutdown were not performed";
    case Text::normal_detail_finished:
        return "The system may delay or reject the action";
    case Text::dry_run_action_prefix:
        return "[Check] ";
    case Text::action_through:
        return " in ";
    case Text::permission_error:
        return "The current user is not allowed to perform this action.\n";
    case Text::sleep_unavailable:
        return "Sleep is not available on this computer.";
    case Text::linux_root:
        return "Run the app as a regular user, not through sudo.";
    case Text::linux_dependencies:
        return "systemd-logind and the busctl and systemctl programs are required.\nThis build "
               "does not support Linux without systemd.";
    case Text::linux_systemd_version:
        return "systemd 248 or newer is required.\n";
    case Text::linux_authorization:
        return "The system requires additional authorisation for this action.\nThe timer was not "
               "started: the app will not request a password when it fires.\nCheck the current "
               "session permissions in system settings.";
    case Text::linux_no_access:
        return "The selected action is currently unavailable or blocked by the system.\n";
    case Text::linux_system_rejected:
        return "The system rejected the request. Check permissions and sleep/shutdown policy.\n\n";
    case Text::linux_timeout:
        return "The system response timed out.\n";
    case Text::linux_timeout_followup:
        return "If the request was already sent, the action may still occur.\n";
    case Text::linux_no_retry:
        return "The request will not be retried automatically.";
    case Text::mac_permission:
        return "macOS did not allow power control.\n";
    case Text::mac_permission_help:
        return "Check the app's automation permissions in System Settings.\n";
    case Text::mac_restart:
        return "Restart the app after changing permissions.\n";
    case Text::mac_rejected:
        return "macOS rejected the request. Code: ";
    case Text::mac_timeout:
        return "macOS did not respond in time. The system may have accepted the request.\n";
    case Text::mac_timeout_followup:
        return "The request will not be retried automatically.";
    case Text::windows_code:
        return "Windows error code: ";
    case Text::mac_code:
        return "macOS error code: ";
    case Text::exit_code:
        return "Exit code: ";
    case Text::process_invalid_parameters:
        return "Invalid system command parameters.";
    case Text::help_text:
        return "Power Timer\n  --dry-run     Never call OS power APIs\n  --smoke-test  Run a "
               "2-second GUI test (requires --dry-run)\n  --lang en|ru  Select the interface "
               "language\n  --license     Show license notices\n";
    case Text::unknown_argument:
        return "Unknown argument: ";
    case Text::smoke_requires_dry_run:
        return "--smoke-test requires --dry-run\n";
    case Text::startup_error:
        return "Could not start the application.\n";
    case Text::language_button:
        return "RU";
    }
    return "";
}

} // namespace power_timer
