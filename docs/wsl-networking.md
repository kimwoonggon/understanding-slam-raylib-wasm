# WSL 네트워크 구조 이해: Linux IP vs Windows LAN IP

이 문서는 WSL2 환경에서 iPad(같은 Wi-Fi)로 접속할 때 왜 IP가 2개처럼 보이는지 설명합니다.

## 핵심 요약

- `Linux IP` (`hostname -I` in WSL): WSL 내부 가상 네트워크 IP (보통 `172.x.x.x`)
- `Windows LAN IP` (`ipconfig` in Windows): 실제 Wi-Fi/LAN 어댑터 IP (보통 `192.168.x.x`, `10.x.x.x`)
- iPad는 WSL 내부 IP로 직접 못 붙고, Windows LAN IP로 붙어야 합니다.

## 왜 그런가

WSL2는 Windows 안에서 NAT 뒤에 있는 별도 가상 머신처럼 동작합니다.

- WSL은 내부망: `172.x.x.x`
- Windows는 외부망(공유기/와이파이)에 직접 연결: `192.168.x.x` 또는 `10.x.x.x`

iPad는 외부망에서 접근하므로 Windows 인터페이스까지만 직접 도달할 수 있습니다.

## 트래픽 흐름

```text
iPad (same Wi-Fi)
   -> Router/LAN
      -> Windows LAN IP:8080
         -> (portproxy)
            -> WSL Linux IP:8080
               -> slam-static-server (WSL)
```

## 각 IP 확인 방법

### WSL 내부 IP (Linux IP)

```bash
hostname -I
```

### Windows LAN IP

Windows PowerShell/CMD:
```powershell
ipconfig
```

또는 WSL에서 helper 사용:
```bash
./scripts/print_access_ip.sh --port 8080
```

## iPad 접속이 안 되는 대표 케이스

1. WSL IP(`172.x.x.x`)로 직접 접속 시도
2. 서버가 `127.0.0.1` 바인딩으로 떠 있음 (`0.0.0.0` 아님)
3. Windows portproxy 미설정
4. Windows 방화벽 인바운드 미허용

## 정석 설정 순서 (WSL2 + iPad)

1. WSL에서 서버 실행:
```bash
./scripts/run_wasm_chrome.sh --host 0.0.0.0 --port 8080
```

2. Windows 관리자 PowerShell에서 포워딩:
```powershell
$wslIp = "172.25.159.91"  # WSL의 hostname -I 값
netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=8080 connectaddress=$wslIp connectport=8080
netsh advfirewall firewall add rule name="WSL 8080" dir=in action=allow protocol=TCP localport=8080
```

3. iPad에서 접속:
```text
http://<Windows LAN IP>:8080/slam-raylib.html
```

## 참고

- WSL 재시작 후 `Linux IP`가 바뀌면 `portproxy`를 갱신해야 할 수 있습니다.
- 확인:
```powershell
netsh interface portproxy show v4tov4
```
