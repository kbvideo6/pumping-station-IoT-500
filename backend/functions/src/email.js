const sgMail = require('@sendgrid/mail');

const apiKey = process.env.SENDGRID_API_KEY;
if (apiKey) {
  sgMail.setApiKey(apiKey);
} else {
  console.warn("SENDGRID_API_KEY environment variable is not set. Emails will only be logged.");
}

exports.sendAlertEmail = async (to, alert) => {
  const alertTypeLabels = {
    'HIGH_CURRENT':  '⚠️ High Current Alert',
    'LOW_CURRENT':   '⚠️ Low Current Alert',
    'NO_CURRENT':    '🔴 No Current Detected',
    'HIGH_VOLTAGE':  '⚠️ High Voltage Alert',
    'LOW_VOLTAGE':   '⚠️ Low Voltage Alert',
    'DEVICE_OFFLINE':'🔴 Device Offline'
  };

  const subject = `${alertTypeLabels[alert.type] || 'Alert'} — ${alert.stationName}`;

  const html = `
    <div style="font-family: Arial, sans-serif; max-width: 600px; padding: 20px; border: 1px solid #ddd; border-radius: 8px; background-color: #fafafa;">
      <h2 style="color: #ef4444; margin-top: 0;">${alertTypeLabels[alert.type] || 'Alert Alerted'}</h2>
      <table style="width: 100%; border-collapse: collapse; margin-bottom: 20px;">
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Station ID</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-family: monospace;">${alert.stationId}</td>
        </tr>
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Station Name</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee;">${alert.stationName}</td>
        </tr>
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Reading</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-family: monospace;">${alert.currentValue !== null ? alert.currentValue + ' A' : 'N/A'}</td>
        </tr>
        ${alert.voltageValue !== undefined && alert.voltageValue !== null ? `
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Voltage</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-family: monospace;">${alert.voltageValue} V</td>
        </tr>` : ''}
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Threshold</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-family: monospace;">${alert.threshold !== null ? alert.threshold + ' A' : 'N/A'}</td>
        </tr>
        <tr>
          <td style="padding: 8px; border-bottom: 1px solid #eee; font-weight: bold;">Time</td>
          <td style="padding: 8px; border-bottom: 1px solid #eee;">${new Date(alert.timestamp || Date.now()).toLocaleString('de-AT', { timeZone: 'Europe/Vienna' })}</td>
        </tr>
      </table>
      <div style="text-align: center; margin-top: 30px;">
        <a href="https://${process.env.GCLOUD_PROJECT || 'pumping-station-iot'}.web.app/#/station/${alert.stationId}" 
           style="background-color: #06b6d4; color: white; padding: 12px 24px; text-decoration: none; border-radius: 6px; font-weight: bold; display: inline-block;">
          View Station Dashboard →
        </a>
      </div>
    </div>
  `;

  const msg = {
    to,
    from: process.env.ALERT_FROM_EMAIL || 'alerts@pumping-station-iot.com',
    subject,
    html
  };

  if (apiKey) {
    try {
      await sgMail.send(msg);
      console.log(`Email alert sent successfully to ${to} for station ${alert.stationId}`);
      return true;
    } catch (error) {
      console.error(`Failed to send email alert to ${to} via SendGrid:`, error);
      if (error.response) {
        console.error(error.response.body);
      }
      return false;
    }
  } else {
    console.log(`[SIMULATED EMAIL] To: ${to} | Subject: ${subject} | Body length: ${html.length} characters`);
    return true;
  }
};
