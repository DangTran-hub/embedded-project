import 'dart:convert';
import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MqttService {
  MqttServerClient? _client;

  final String broker = "g634f918.ala.asia-southeast1.emqxsl.com";
  final int port = 8883;
  final String username = "smartlock_user";
  final String password = ".enZ22A6L.9L_yY";

  final Set<String> _subscribedTopics = {};
  FutureOr<void> Function(String lockId, Map<String, dynamic> data)? onMessage;
  bool _isConnecting = false;

  Future<bool> connect() async {
    // 1. Kiểm tra nếu đang kết nối hoặc đã kết nối rồi
    if (_isConnecting) return false;
    if (_client?.connectionStatus?.state == MqttConnectionState.connected) {
      return true;
    }

    _isConnecting = true;
    final clientId =
        "oppo_lock_${DateTime.now().millisecondsSinceEpoch.toString().substring(10)}";

    _client = MqttServerClient(broker, clientId);
    _client!
      ..port = port
      ..secure =
          true // Bắt buộc true cho port 8883
      ..keepAlivePeriod = 60
      ..connectTimeoutPeriod = 20000
      ..autoReconnect = true
      ..setProtocolV311()
      ..onDisconnected = _onDisconnected
      ..onConnected = () => debugPrint('MQTT Connected Successfully');

    // Cho phép chứng chỉ tự ký (rất quan trọng cho Flutter Mobile)
    _client!.onBadCertificate = (dynamic cert) => true;

    final connMess = MqttConnectMessage()
        .withClientIdentifier(clientId)
        .authenticateAs(username, password)
        .startClean(); // startClean giúp không nhận lại tin nhắn cũ bị dồn ứ

    _client!.connectionMessage = connMess;

    try {
      debugPrint('Dang ket noi MQTT...');
      await _client!.connect();

      if (_client?.connectionStatus?.state == MqttConnectionState.connected) {
        _client!.updates!.listen(_handleMessage);
        return true;
      }
    } catch (e) {
      debugPrint('Loi ket noi MQTT: $e');
      _client?.disconnect();
    } finally {
      _isConnecting = false;
    }
    return false;
  }

  void _onDisconnected() {
    debugPrint('MQTT Disconnected');
    _subscribedTopics.clear();
    _isConnecting = false;
  }

  Future<void> subscribeLock(String lockId) async {
    bool success = await connect();
    if (success) {
      final topic = "smartlock/$lockId/status";
      if (_subscribedTopics.contains(topic)) return;
      _client!.subscribe(topic, MqttQos.atLeastOnce);
      _subscribedTopics.add(topic);
      debugPrint('Subscribed to: $topic');
    }
  }

  Future<void> publish(String topic, String payload) async {
    bool success = await connect();
    if (success) {
      final builder = MqttClientPayloadBuilder();
      builder.addString(payload);
      _client!.publishMessage(topic, MqttQos.atLeastOnce, builder.payload!);
      debugPrint('MQTT Sent: $topic -> $payload');
    }
  }

  Future<void> sendCommand(String lockId, bool lock, String by) async {
    final topic = "smartlock/$lockId/cmd";
    final payload = jsonEncode({"action": lock ? "lock" : "unlock", "by": by});
    await publish(topic, payload);
  }

  void _handleMessage(List<MqttReceivedMessage<MqttMessage>> events) {
    try {
      final recMsg = events[0].payload as MqttPublishMessage;
      final topic = events[0].topic;
      final payload = MqttPublishPayload.bytesToStringAsString(
        recMsg.payload.message,
      );

      debugPrint('MQTT Received: $topic -> $payload');

      final data = jsonDecode(payload) as Map<String, dynamic>;
      final List<String> parts = topic.split('/');
      if (parts.length < 2) return;

      final String lockId = parts[1];

      // Chuẩn hóa dữ liệu để LockNotifier (Legacy) không bị lỗi
      if (data.containsKey('locked') ||
          data.containsKey('battery') ||
          data.containsKey('online')) {
        data['isOnline'] = data['online'] ?? true;
        data['isLocked'] = data['locked'] ?? true;
        // Xử lý battery nếu ESP32 gửi dạng double
        if (data.containsKey('battery')) {
          data['battery'] = (data['battery'] as num).toInt();
        }
      }

      // Callback về Provider
      unawaited(Future<void>.value(onMessage?.call(lockId, data)));
    } catch (e) {
      debugPrint('Loi xu ly tin nhan MQTT: $e');
    }
  }

  void unsubscribeAll() {
    debugPrint('Ngat ket noi toan bo MQTT');
    _client?.disconnect();
    _client = null;
    _subscribedTopics.clear();
  }
}

final mqttService = MqttService();
