use sha2::{Sha256, Digest};

fn fib(n: usize) -> usize {
    let mut a = 0;
    let mut b = 1;
    for _ in 0..n {
        let temp = a;
        a = b;
        b = temp + b;
    }
    a
}

fn cryptographic_fibonacci_braid(input: &str) -> String {
    let mut hasher = Sha256::new();
    hasher.update(input.as_bytes());
    let salt = hasher.finalize();

    let chars: Vec<char> = input.chars().collect();
    let mut left: Vec<char> = chars.iter().enumerate()
        .filter(|(i, _)| i % 2 == 0).map(|(_, &c)| c).collect();
    let mut right: Vec<char> = chars.iter().enumerate()
        .filter(|(i, _)| i % 2 != 0).map(|(_, &c)| c).collect();

    let mut output = String::new();
    let mut n = 1;
    let forbidden = [':', '(', ')', '[', ']', '{', '}', '=', '"', '\'', ','];

    while !left.is_empty() || !right.is_empty() {
        let len_l = if left.is_empty() { 0 } else { fib(n) % (left.len() + 1) };
        let len_r = if right.is_empty() { 0 } else { fib(n + 1) % (right.len() + 1) };

        for _ in 0..len_l {
            if let Some(c) = left.pop() {
                if !forbidden.contains(&c) { output.push(c); }
            }
        }
        for _ in 0..len_r {
            if let Some(c) = right.pop() {
                if !forbidden.contains(&c) { output.push(c); }
            }
        }
        n += 1;
    }
    output
}

fn reverse_fibonacci_braid(agnostic_stream: &str, original_len: usize) -> String {
    let stream_chars: Vec<char> = agnostic_stream.chars().collect();
    let mut left = Vec::new();
    let mut right = Vec::new();

    let mut cursor = 0;
    let mut n = 1;

    let orig_l_len = original_len / 2;
    let orig_r_len = original_len - orig_l_len;

    while cursor < stream_chars.len() {
        let len_l = fib(n) % (orig_l_len + 1);
        let len_r = fib(n + 1) % (orig_r_len + 1);

        for _ in 0..len_l {
            if cursor < stream_chars.len() {
                left.push(stream_chars[cursor]);
                cursor += 1;
            }
        }
        for _ in 0..len_r {
            if cursor < stream_chars.len() {
                right.push(stream_chars[cursor]);
                cursor += 1;
            }
        }
        n += 1;
    }

    left.reverse();
    right.reverse();

    let mut result = String::new();
    let mut i = 0;
    while i < left.len() || i < right.len() {
        if i < left.len() { result.push(left[i]); }
        if i < right.len() { result.push(right[i]); }
        i += 1;
    }
    result
}

fn lossless_braid(input: &str) -> (String, u32) {
    let mut hasher = Sha256::new();
    hasher.update(input.as_bytes());
    let hash_result = hasher.finalize();
    let kappa = hash_result[0] as u32;

    let substituted: Vec<char> = input.chars()
        .map(|c| std::char::from_u32((c as u32 + kappa)).unwrap_or(c))
        .collect();

    let mut left: Vec<char> = substituted.iter().enumerate()
        .filter(|(i, _)| i % 2 == 0).map(|(_, &c)| c).collect();
    let mut right: Vec<char> = substituted.iter().enumerate()
        .filter(|(i, _)| i % 2 != 0).map(|(_, &c)| c).collect();

    let mut output = String::new();
    let mut n = 1;
    while !left.is_empty() || !right.is_empty() {
        let len_l = if left.is_empty() { 0 } else { fib(n) % (left.len() + 1) };
        let len_r = if right.is_empty() { 0 } else { fib(n + 1) % (right.len() + 1) };

        for _ in 0..len_l { if let Some(c) = left.pop() { output.push(c); } }
        for _ in 0..len_r { if let Some(c) = right.pop() { output.push(c); } }
        n += 1;
    }
    (output, kappa)
}

fn lossless_reverse(braided: &str, kappa: u32, original_len: usize) -> String {
    let stream_chars: Vec<char> = braided.chars().collect();
    let mut left = Vec::new();
    let mut right = Vec::new();
    let mut cursor = 0;
    let mut n = 1;
    let orig_l_len = original_len / 2;
    let orig_r_len = original_len - orig_l_len;

    while cursor < stream_chars.len() {
        let len_l = fib(n) % (orig_l_len + 1);
        let len_r = fib(n + 1) % (orig_r_len + 1);
        for _ in 0..len_l { if cursor < stream_chars.len() { left.push(stream_chars[cursor]); cursor += 1; } }
        for _ in 0..len_r { if cursor < stream_chars.len() { right.push(stream_chars[cursor]); cursor += 1; } }
        n += 1;
    }

    left.reverse();
    right.reverse();

    let mut restored = String::new();
    for i in 0..orig_l_len.max(orig_r_len) {
        if i < left.len() { restored.push(left[i]); }
        if i < right.len() { restored.push(right[i]); }
    }

    restored.chars()
        .map(|c| std::char::from_u32((c as u32).wrapping_sub(kappa)).unwrap_or(c))
        .collect()
}

fn main() {
    let code = "def hello():\n    print('World!')";

    println!("=== LOSSY BRAID (with filter) ===");
    let braided = cryptographic_fibonacci_braid(code);
    println!("Original:  {}", code);
    println!("Braided:   {}", braided);
    println!("Salt:      {:02x?}", Sha256::new().chain_update(code.as_bytes()).finalize()[..4]);

    println!("\n=== LOSSLESS BRAID (with substitution) ===");
    let len = code.len();
    let (lossless, kappa) = lossless_braid(code);
    println!("Original:  {}", code);
    println!("Braided:   {}", lossless);
    println!("Kappa:     {}", kappa);

    let recovered = lossless_reverse(&lossless, kappa, len);
    println!("Recovered: {}", recovered);
    assert_eq!(code, recovered);
    println!("Lossless round-trip: PASS");

    println!("\n=== REVERSE BRAID (from agnostic stream) ===");
    let partial = reverse_fibonacci_braid(&braided, code.len());
    println!("Recovered (no structural tokens): {}", partial);
}
