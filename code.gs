function doGet(e) {
  // ถ้ามีพารามิเตอร์ mode=getData → ส่งข้อมูล JSON ไปยังหน้าเว็บ
  if (e.parameter && e.parameter.mode === "getData") {
    return getFilteredData(e.parameter.range);
  }

  // ถ้ามีพารามิเตอร์ temp/hum/status → รับข้อมูลจาก ESP8266
  if (e.parameter && e.parameter.temp && e.parameter.hum && e.parameter.status) {
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    const timestamp = new Date();
    const temp = e.parameter.temp;
    const hum = e.parameter.hum;
    const status = e.parameter.status;
    sheet.appendRow([timestamp, temp, hum, status]);
    return ContentService.createTextOutput("Data recorded");
  }

  // ถ้าไม่มีพารามิเตอร์ → แสดงหน้า dashboard.html
  return HtmlService.createHtmlOutputFromFile("dashboard");
}

function getFilteredData(range) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const data = sheet.getDataRange().getValues();
  const headers = data[0];
  const rows = data.slice(1);

  let filtered = [];

  if (range === "latest") {
    filtered = rows.slice(-120); // ✅ ดึง 120 รายการล่าสุด
  } else {
    const now = new Date();
    filtered = rows.filter(row => {
      const timestamp = new Date(row[0]);
      if (isNaN(timestamp)) return false;

      switch (range) {
        case "today":
          return timestamp.toDateString() === now.toDateString();
        case "week":
          const weekAgo = new Date(now);
          weekAgo.setDate(now.getDate() - 7);
          return timestamp >= weekAgo;
        case "month":
          return timestamp.getMonth() === now.getMonth() &&
                 timestamp.getFullYear() === now.getFullYear();
        case "year":
          return timestamp.getFullYear() === now.getFullYear();
        default:
          return true;
      }
    });
  }

  return ContentService.createTextOutput(JSON.stringify(
    filtered.map(row => ({
      Timestamp: row[0],
      Temperature: row[1],
      Humidity: row[2],
      Status: row[3]
    }))
  )).setMimeType(ContentService.MimeType.JSON);
}
