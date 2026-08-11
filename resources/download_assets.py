# resources/download_assets.py

import os
import sys
import json
import urllib.request


class ResourceElement:
    def __init__(self, url: str, destination_path: str):
        self.url = url
        self.destination_path = destination_path

    def download(self):
        if self.__is_exists():
            print(f"{self.destination_path} already exists. Skipping download.")
            return

        self.__create_directory()

        file_data = self.__request_file()

        if not file_data:
            print(f"Failed to download {self.url}. Exiting.", file=sys.stderr)
            sys.exit(1)

        self.__save(file_data)

    def __request_file(self) -> bytes:
        """지정된 URL에서 파일을 요청하고 바이트로 반환합니다."""
        print(f"Downloading {self.url} to {self.destination_path}...")

        try:
            request = urllib.request.Request(
                self.url,
                headers={
                    "User-Agent": (
                        "Mozilla/5.0 (X11; Linux x86_64) "
                        "AppleWebKit/537.36 (KHTML, like Gecko) "
                        "Chrome/138.0.0.0 Safari/537.36"
                    ),
                    "Accept": (
                        "text/html,application/xhtml+xml,application/xml;q=0.9,"
                        "image/avif,image/webp,image/apng,*/*;q=0.8"
                    ),
                    "Accept-Language": "en-US,en;q=0.9",
                },
            )

            with urllib.request.urlopen(request, timeout=30) as response:
                return response.read()

        except urllib.error.HTTPError as e:
            print(
                f"HTTP error requesting {self.url}: "
                f"{e.code} {e.reason}",
                file=sys.stderr,
            )
            sys.exit(1)

        except urllib.error.URLError as e:
            print(
                f"URL error requesting {self.url}: {e.reason}",
                file=sys.stderr,
            )
            sys.exit(1)

        except Exception as e:
            print(
                f"Error requesting {self.url}: {e}",
                file=sys.stderr,
            )
            sys.exit(1)

    def __create_directory(self):
        """destination_path의 디렉터리가 존재하지 않으면 생성합니다."""
        dir_name = os.path.dirname(self.destination_path)

        if dir_name and not os.path.exists(dir_name):
            os.makedirs(dir_name, exist_ok=True)

    def __is_exists(self) -> bool:
        """destination_path에 파일이 이미 존재하는지 확인합니다."""
        return os.path.exists(self.destination_path)

    def __save(self, file_data: bytes):
        """다운로드한 데이터를 destination_path에 저장합니다."""
        try:
            with open(self.destination_path, "wb") as out_file:
                out_file.write(file_data)

            print(f"Download complete: {self.destination_path}")

        except Exception as e:
            print(
                f"Error saving {self.destination_path}: {e}",
                file=sys.stderr,
            )
            sys.exit(1)


def main():
    # 현재 스크립트가 위치한 resources 디렉터리
    resources_dir = os.path.dirname(os.path.abspath(__file__))

    resources_file = os.path.join(
        resources_dir,
        "resources.json",
    )

    if not os.path.exists(resources_file):
        print(
            f"Resources file {resources_file} does not exist.",
            file=sys.stderr,
        )
        sys.exit(1)

    try:
        with open(resources_file, "r", encoding="utf-8") as f:
            resources_data = json.load(f)

    except Exception as e:
        print(
            f"Failed to read {resources_file}: {e}",
            file=sys.stderr,
        )
        sys.exit(1)

    texture_items = resources_data.get("textures", [])
    model_items = resources_data.get("models", [])

    textures = [
        ResourceElement(
            url=item["url"],
            destination_path=os.path.join(
                resources_dir,
                item["destination_path"],
            ),
        )
        for item in texture_items
    ]

    models = [
        ResourceElement(
            url=item["url"],
            destination_path=os.path.join(
                resources_dir,
                item["destination_path"],
            ),
        )
        for item in model_items
    ]

    for resource in textures + models:
        resource.download()


if __name__ == "__main__":
    main()

