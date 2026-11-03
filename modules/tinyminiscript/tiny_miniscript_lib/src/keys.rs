//! Translate the Core harness's symbolic key indexes, only in key positions.
use bitcoin::secp256k1::{PublicKey, Secp256k1, SecretKey};
use std::sync::OnceLock;

fn keys() -> &'static Vec<PublicKey> {
    static KEYS: OnceLock<Vec<PublicKey>> = OnceLock::new();
    KEYS.get_or_init(|| {
        let secp = Secp256k1::new();
        (0..=255)
            .map(|index| {
                let mut bytes = [0; 32];
                bytes[0] = 1;
                bytes[31] = index;
                PublicKey::from_secret_key(&secp, &SecretKey::from_slice(&bytes).unwrap())
            })
            .collect()
    })
}

pub fn translate(input: &str, tap: bool) -> Option<String> {
    let mut output = String::with_capacity(input.len());
    let mut calls: Vec<(&str, usize)> = Vec::new();
    let mut start = 0;
    for (end, delimiter) in input
        .char_indices()
        .chain(std::iter::once((input.len(), '\0')))
    {
        if !matches!(delimiter, '(' | ')' | ',' | ':' | '\0') {
            continue;
        }
        let token = &input[start..end];
        let key_position = calls.last().is_some_and(|(name, argument)| {
            matches!(*name, "pk" | "pkh" | "pk_k" | "pk_h") && *argument == 0
                || matches!(*name, "multi" | "multi_a") && *argument > 0
        });
        if key_position {
            if token.is_empty() || token.len() > 2 || !token.bytes().all(|b| b.is_ascii_hexdigit())
            {
                return None;
            }
            let index = u8::from_str_radix(token, 16).ok()?;
            let key = &keys()[index as usize];
            output.push_str(&if tap {
                key.x_only_public_key().0.to_string()
            } else {
                key.to_string()
            });
        } else {
            output.push_str(token);
        }
        match delimiter {
            '(' => calls.push((token, 0)),
            ',' => calls.last_mut()?.1 += 1,
            ')' => {
                calls.pop()?;
            }
            _ => (),
        }
        if delimiter != '\0' {
            output.push(delimiter);
        }
        start = end + delimiter.len_utf8();
    }
    if !calls.is_empty() {
        return None;
    }
    Some(output)
}
