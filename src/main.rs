use anyhow::Result;
use blobstore_rs::ffi::put_async;
use blobstore_rs::ffi::{new_blobstore_client, MultiBuf, VecU8};
#[tokio::main]
async fn main() -> Result<()> {
    let create_buf = |str1: &[u8], str2: &[u8]| -> MultiBuf {
        let chunks = vec![
            VecU8 {
                value: str1.to_vec(),
            },
            VecU8 {
                value: str2.to_vec(),
            },
        ];
        MultiBuf { chunks, pos: 0 }
    };

    let client = new_blobstore_client();
    let mut buf1 = create_buf(b"fearless", b"concurrency");
    let mut buf2 = create_buf(b"iterators", b"closures");
    let (id1, id2) = tokio::join!(put_async(&client, &mut buf1), put_async(&client, &mut buf2));

    let print_meta = |blobid: u64, tag: &str| {
        client.tag(blobid, tag);
        let metadata = client.metadata(blobid);
        println!("tags = {:?}", metadata.tags);
    };
    if let Ok(id) = id1 {
        print_meta(id, "threads");
    };
    if let Ok(id) = id2 {
        print_meta(id, "functional");
    };
    Ok(())
}
