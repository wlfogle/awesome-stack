use reqwest;
use std::time::Duration;
use tokio;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("Testing connection to ct-900:11434...");
    
    let client = reqwest::Client::builder()
        .timeout(Duration::from_secs(10))
        .build()?;
    
    let ai_container_url = "http://ct-900:11434/api/tags";
    
    match client.get(ai_container_url).send().await {
        Ok(response) => {
            println!("✅ Connection successful!");
            println!("Status: {}", response.status());
            if response.status().is_success() {
                let text = response.text().await?;
                println!("Response: {}", &text[..std::cmp::min(200, text.len())]);
            }
        }
        Err(e) => {
            println!("❌ Connection failed: {}", e);
            println!("Error details: {:#?}", e);
        }
    }
    
    Ok(())
}
