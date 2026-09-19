#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Locale {
    En,
    Ru,
}

#[derive(Clone, Copy)]
pub struct Strings {
    pub app_title: &'static str,
    pub subtitle: &'static str,
    pub subtitle_dry: &'static str,
    pub sleep: &'static str,
    pub shutdown: &'static str,
    pub through: &'static str,
    pub hours: &'static str,
    pub minutes: &'static str,
    pub presets: [&'static str; 4],
    pub not_started: &'static str,
    pub invalid: &'static str,
    pub sleep_detail: &'static str,
    pub shutdown_detail: &'static str,
    pub start: &'static str,
    pub cancel: &'static str,
    pub cancelled: &'static str,
    pub checking: &'static str,
    pub checking_detail: &'static str,
    pub started: &'static str,
    pub interrupted: &'static str,
    pub interrupted_detail: &'static str,
    pub sending: &'static str,
    pub unavailable_cancel: &'static str,
    pub finished: &'static str,
    pub submitted: &'static str,
    pub failed: &'static str,
    pub dry_footer: &'static str,
    pub normal_footer: &'static str,
    pub language: &'static str,
}

pub fn strings(locale: Locale) -> Strings {
    match locale {
        Locale::En => Strings {
            app_title: "Power Timer",
            subtitle: "Sleep or shut down on a timer",
            subtitle_dry: "Safe check without changing power state",
            sleep: "Sleep",
            shutdown: "Shut down",
            through: "In",
            hours: "h",
            minutes: "min",
            presets: ["15 min", "30 min", "1 hour", "2 hours"],
            not_started: "Timer is not running",
            invalid: "Enter an interval from 1 minute to 24 hours",
            sleep_detail: "Sleep mode, not hibernation",
            shutdown_detail: "Save your documents before shutdown",
            start: "Start timer",
            cancel: "Cancel timer",
            cancelled: "Timer cancelled",
            checking: "Checking whether the action is available",
            checking_detail: "The countdown will start after the check",
            started: "Timer started",
            interrupted: "Timer cancelled after an interruption",
            interrupted_detail: "Sleep, a clock change, or a pause longer than 10 seconds",
            sending: "Sending request to the system",
            unavailable_cancel: "Cancellation is no longer available",
            finished: "Check completed",
            submitted: "Request submitted to the system",
            failed: "Action was not completed",
            dry_footer: "Check mode: sleep and shutdown are disabled.",
            normal_footer: "You can minimise the window. Closing cancels the timer.",
            language: "RU",
        },
        Locale::Ru => Strings {
            app_title: "Таймер питания",
            subtitle: "Сон или выключение по таймеру",
            subtitle_dry: "Проверка без изменения питания",
            sleep: "Спящий режим",
            shutdown: "Выключить",
            through: "Через",
            hours: "ч",
            minutes: "мин",
            presets: ["15 мин", "30 мин", "1 час", "2 часа"],
            not_started: "Таймер не запущен",
            invalid: "Укажите интервал от 1 минуты до 24 часов",
            sleep_detail: "Спящий режим, не гибернация",
            shutdown_detail: "Сохраните документы до выключения",
            start: "Запустить таймер",
            cancel: "Отменить таймер",
            cancelled: "Таймер отменён",
            checking: "Проверка доступности действия",
            checking_detail: "После проверки начнётся отсчёт",
            started: "Таймер запущен",
            interrupted: "Таймер отменён после перерыва",
            interrupted_detail: "Сон, изменение часов или пауза дольше 10 секунд",
            sending: "Отправка запроса системе",
            unavailable_cancel: "На этом этапе отмена уже недоступна",
            finished: "Проверка завершена",
            submitted: "Запрос передан системе",
            failed: "Действие не выполнено",
            dry_footer: "Режим проверки: сон и выключение отключены.",
            normal_footer: "Можно свернуть окно. Закрытие отменяет таймер.",
            language: "EN",
        },
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn both_catalogs_are_populated() {
        for locale in [Locale::En, Locale::Ru] {
            let s = strings(locale);
            assert!(!s.app_title.is_empty());
            assert!(s.presets.iter().all(|value| !value.is_empty()));
        }
    }
}
