use rand::seq::SliceRandom;
use rand::thread_rng;
use std::collections::HashMap;
use std::fs;
use std::time::Instant;

fn get_peak_memory_kb() -> usize {
    #[cfg(target_os = "windows")]
    {
        use windows_sys::Win32::System::ProcessStatus::{
            GetProcessMemoryInfo, PROCESS_MEMORY_COUNTERS,
        };
        use windows_sys::Win32::System::Threading::GetCurrentProcess;

        unsafe {
            let mut pmc: PROCESS_MEMORY_COUNTERS = std::mem::zeroed();
            pmc.cb = std::mem::size_of::<PROCESS_MEMORY_COUNTERS>() as u32;
            if GetProcessMemoryInfo(GetCurrentProcess(), &mut pmc, pmc.cb) != 0 {
                return (pmc.PeakWorkingSetSize / 1024) as usize;
            }
        }
        0
    }

    #[cfg(not(target_os = "windows"))]
    {
        unsafe {
            let mut usage: libc::rusage = std::mem::zeroed();
            libc::getrusage(libc::RUSAGE_SELF, &mut usage);
            usage.ru_maxrss as usize
        }
    }
}

fn ends_with_punctuation(word: &str) -> bool {
    word.ends_with('.') || word.ends_with('!') || word.ends_with('?')
}

fn main() {
    let filename = "../text_1000.txt";
    let text = match fs::read_to_string(filename) {
        Ok(content) => content,
        Err(_) => {
            eprintln!("Error reading file: {}", filename);
            return;
        }
    };

    let words: Vec<&str> = text.split_whitespace().collect();
    if words.len() < 2 {
        eprintln!("Text is too short.");
        return;
    }

    // Початок вимірювання часу
    let start_time = Instant::now();

    // 1. Побудова Марковського ланцюга
    let mut chain: HashMap<&str, Vec<&str>> = HashMap::new();
    let mut start_words: Vec<&str> = Vec::new();

    for i in 0..words.len() - 1 {
        chain.entry(words[i]).or_default().push(words[i + 1]);

        if words[i].chars().next().map_or(false, |c| c.is_uppercase()) {
            start_words.push(words[i]);
        }
    }

    if start_words.is_empty() {
        start_words.push(words[0]);
    }

    // 2. Генерація речення
    let mut rng = thread_rng();
    let mut current_word = *start_words.choose(&mut rng).unwrap();
    let mut sentence_words = vec![current_word];

    let max_words = 30;

    while !ends_with_punctuation(current_word) && sentence_words.len() < max_words {
        if let Some(next_options) = chain.get(current_word) {
            current_word = *next_options.choose(&mut rng).unwrap();
            sentence_words.push(current_word);
        } else {
            break;
        }
    }

    let mut sentence = sentence_words.join(" ");
    if !ends_with_punctuation(current_word) {
        sentence.push('.');
    }

    // Кінець вимірювання часу
    let duration = start_time.elapsed();
    let memory_kb = get_peak_memory_kb();

    println!("--- Generated Sentence (Rust) ---");
    println!("{}\n", sentence);
    println!("--- Performance Results ---");
    println!("Execution time: {:.3} ms", duration.as_secs_f64() * 1000.0);
    println!("Peak memory usage: {} KB", memory_kb);
}